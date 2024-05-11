#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "bufferdialog.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTimer>
#include <QStyleFactory>

#define NO_ERR 0
#define WRITABLE 1
#define NOT_WRITABLE 2

#define SERIAL_SPEED QSerialPort::Baud115200

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    serialPort(new QSerialPort),
    bufferDialog(new BufferDialog(this))
{
    ui->setupUi(this);

    QObject::connect(this, SIGNAL(chipUpdated()), this, SLOT(resizeBuffers()));
    //QObject::connect(this, SIGNAL(bufferUpdated()), this, SLOT(updateButtons()));

    updatePortsConnection = QObject::connect(&updatePortsTimer, SIGNAL(timeout()), this, SLOT(reload_ports()));
    updatePortsTimer.setInterval(1000);
    updatePortsTimer.start();
    reload_ports();

    bufferDialog->setVisible(false);

    ui->progressBar->setStyle(QStyleFactory::create("Fusion"));
    ui->progressBar->setValue(0);
    ui->progressBar->setTextVisible(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::log(QString str)
{
    ui->textBrowser->append(str);
    ui->textBrowser->moveCursor(QTextCursor::End);
}

void MainWindow::chipSelectSetEnabled (bool state)
{
    ui->c16Button->setEnabled(state);
    ui->c32Button->setEnabled(state);
    ui->c64Button->setEnabled(state);
    ui->c128Button->setEnabled(state);
    ui->c256Button->setEnabled(state);
    ui->c512Button->setEnabled(state);
    ui->c1001Button->setEnabled(state);
    ui->c2001Button->setEnabled(state);
    ui->c4001Button->setEnabled(state);
    ui->c801Button->setEnabled(state);

    // Clear radio buttons for correct work after reconnect
    ui->c16Button->setAutoExclusive(state);
    ui->c32Button->setAutoExclusive(state);
    ui->c64Button->setAutoExclusive(state);
    ui->c128Button->setAutoExclusive(state);
    ui->c256Button->setAutoExclusive(state);
    ui->c512Button->setAutoExclusive(state);
    ui->c1001Button->setAutoExclusive(state);
    ui->c2001Button->setAutoExclusive(state);
    ui->c4001Button->setAutoExclusive(state);
    ui->c801Button->setAutoExclusive(state);
    if (!state){
        ui->c16Button->setChecked(false);
        ui->c32Button->setChecked(false);
        ui->c64Button->setChecked(false);
        ui->c128Button->setChecked(false);
        ui->c256Button->setChecked(false);
        ui->c512Button->setChecked(false);
        ui->c1001Button->setChecked(false);
        ui->c2001Button->setChecked(false);
        ui->c4001Button->setChecked(false);
        ui->c801Button->setChecked(false);
    }
}

void MainWindow::on_connectButton_clicked()
{
    QListWidgetItem* item = ui->portList->currentItem();
    if (item == NULL) {
        QMessageBox::critical(this, tr("EPROM Programmer"), tr("Select serial port!"));
        return;
    }
    if (!(item->flags() & Qt::ItemIsSelectable)) {
        QMessageBox::critical(this, tr("EPROM Programmer"), tr("Port is busy!"));
        return;
    }
    log(QString("Connect to %1").arg(item->data(Qt::UserRole).toString()));
    openSerialPort(item->data(Qt::UserRole).toString());
}

void MainWindow::openSerialPort(QString path)
{
    serialPort->setPortName(path);
    serialPort->setBaudRate(SERIAL_SPEED);
    if (serialPort->open(QIODevice::ReadWrite)) {
        QByteArray readData = serialPort->readAll();
        writeData("x");

        while (serialPort->waitForReadyRead(2000)){
            readData.append(serialPort->readAll());
            if (readData.indexOf("Arduino 27 Series programmer V2", 0) != -1){
                log(QString("Connect successful. Received [%1]").arg(QString(readData)));

                //QObject::connect(serialPort, SIGNAL(errorOccurred()), this, SLOT(closeSerialPort()));

                chipSelectSetEnabled(true);
                ui->updateButton->setEnabled(false);
                ui->disconnectButton->setEnabled(true);
                ui->connectButton->setEnabled(false);
                ui->voltageChipButton->setEnabled(true);

                updateButtons(true, false);
                return;
            }
        }
        log(QString("Arduino programmer not found."));
        closeSerialPort();
    } else {
        QMessageBox::critical(this, tr("Error"), serialPort->errorString());
    }
}


void MainWindow::on_disconnectButton_clicked()
{
    closeSerialPort();
}

void MainWindow::closeSerialPort()
{
    chipSelected = false;
    bufferClear = true;

    ui->disconnectButton->setEnabled(false);

    ui->updateButton->setEnabled(false);
    ui->connectButton->setEnabled(false);
    updatePortsTimer.start();

    updateButtons(false, false);
    chipSelectSetEnabled(false);

    ui->showButton->setChecked(false);
    bufferDialog->setVisible(false);

    if (serialPort->isOpen()){
        serialPort->close();
        log(QString("Disconnect..."));
        QObject::disconnect(serialDataConnection);
    }
}


void MainWindow::on_updateButton_clicked()
{
    ui->updateButton->setEnabled(false);
    ui->connectButton->setEnabled(false);
    reload_ports();
    updatePortsTimer.start();
}

void MainWindow::on_portList_itemClicked(QListWidgetItem *item)
{
    (void)item;
    ui->connectButton->setEnabled(true);
    ui->updateButton->setEnabled(true);
    updatePortsTimer.stop();
}

void MainWindow::reload_ports()
{
    ui->portList->clear();
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        QListWidgetItem *item = new QListWidgetItem(info.portName(), ui->portList);
        item->setData(Qt::UserRole, info.systemLocation());
        if (info.isBusy()){
            item->setText(info.portName() + " (Busy)");
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        }
    }
}


void MainWindow::updateButtons(bool actions, bool buffer)
{
    if (actions){
        ui->writeChipButton->setEnabled(!bufferClear);
        ui->verifyChipButton->setEnabled(!bufferClear);
        ui->readChipButton->setEnabled(chipSelected);
    } else {
        ui->writeChipButton->setEnabled(false);
        ui->verifyChipButton->setEnabled(false);
        ui->readChipButton->setEnabled(false);
    }
    ui->voltageChipButton->setEnabled(actions || updateVoltageConnection);

    if (buffer){
        ui->saveFileButton->setEnabled(!bufferClear);
        ui->showButton->setEnabled(!bufferClear);
    } else {
        ui->saveFileButton->setEnabled(false);
        ui->showButton->setEnabled(false);
    }
    ui->openFileButton->setEnabled(buffer);
}


void MainWindow::on_writeChipButton_clicked()
{
    writeData("w");
    writeChip();
}

void MainWindow::writeData(const QByteArray &data)
{
    serialPort->write(data);
}

void MainWindow::writeChip()
{
    QByteArray data;
    QByteArray readData;
    int index;

    log(QString("Writing %1 bytes to chip...").arg(bufSize));
    ui->progressBar->setMaximum(bufSize);
    ui->progressBar->setTextVisible(true);

    data.clear();

    for (uint32_t i = 0; i < bufSize; i++){
        data.append(bufWork[i]);
        if ((i & 0x0000001f) == 0x0000001f){
            // 32 byte block
            ui->progressBar->setValue(i + 1);
            qApp->processEvents();

            while (((index = readData.indexOf("Waiting for data")) == -1)
                   && (serialPort->waitForReadyRead(400))){
                readData.append(serialPort->readAll());
            }

            if (index == -1){
                log(QString("Waiting error (programmer not ready?) [%1]").arg(QString(readData)));
                serialPort->waitForReadyRead(5000);
                readData = serialPort->readAll();
                return;
            }
            else{
                int index2 = readData.indexOf("\n", index);
                if (index2 != -1) {
                    readData.remove(0, index2+1);
                }
            }

            writeData(data);
            data.clear();
            qDebug("Sent block 0x%08X-0x%08X", (i - 31), i);

            while (((index = readData.indexOf("Write block ")) == -1)
                   && (serialPort->waitForReadyRead((bufSize == 0x0800) ? 640 : 400))){
                readData.append(serialPort->readAll());
            }

            if (index == -1){
                log(QString("Writing error [%1] (%2)").arg(QString(readData)).arg(serialPort->error()));
                serialPort->waitForReadyRead(1000);
                readData = serialPort->readAll();
                return;
            }
            else{
                int index2 = readData.indexOf("\n", index);
                if (index2 != -1) {
                    readData.remove(0, index2+1);
                }
            }
        }
    }

    while (((index = readData.indexOf("Programming Done")) == -1)
           && (serialPort->waitForReadyRead((bufSize == 0x0800) ? 320 : 200))){
        readData.append(serialPort->readAll());
    }

    if (index == -1){
        log(QString("Programming error [%1]").arg(QString(readData)));
        return;
    }
    else{
        log(QString("Programming done [%1]").arg(QString(readData)));
        int index2 = readData.indexOf("\n", index);
        if (index2 >= 0) {
            readData.remove(index, index2-index+1);
        }
    }
}


void MainWindow::on_readChipButton_clicked()
{
    bufferClear = false;
    bufCheck.fill(0);
    readChip();
    checkClearConnection = QObject::connect(this, SIGNAL(chipRead()), this, SLOT(checkClear()));
}

void MainWindow::readChip()
{
    updateButtons(false, false);
    log(QString("Reading %1 bytes from chip...").arg(bufSize));

    // clear serial buffer
    QByteArray data = serialPort->readAll();

    ui->progressBar->setMaximum(bufSize);
    ui->progressBar->setTextVisible(true);

    serialDataConnection = QObject::connect(serialPort, SIGNAL(readyRead()), this, SLOT(readData()));
    writeData("r");
}

void MainWindow::readData()
{
    static uint32_t count = 0;
    const QByteArray data = serialPort->readAll();
    if (data.count()){
        memcpy(&(bufWork.data())[count], data.data(), data.count());
        count += data.count();
    }
    if (count < bufSize){
        ui->progressBar->setValue(count);
    }
    else {
        ui->progressBar->setValue(bufSize);
        QObject::disconnect(serialDataConnection);
        updateButtons(true, true);

        log(QString("Read %1 bytes.").arg(count));
        count = 0;

        emit chipRead();
        emit bufferUpdated();
    }
}

void MainWindow::checkClear()
{
    uint32_t count;

    QObject::disconnect(checkClearConnection);
    for (count = 0; count < bufSize; count++){
        if (bufWork[count] != (char)0xff){
            log(QString("Chip not clear. Check before write (index of first non-0xFF value: %1)").arg(count));
            break;
        }
    }
    if (count >= bufSize)
        log(QString("Chip clear."));
}


void MainWindow::on_openFileButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open binary to buffer"), "",
                                                    tr("Binary (*.bin);;All Files (*)"));

    if (fileName.isEmpty())
        return;
    else {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::information(this, tr("Unable to open file"),
                                     file.errorString());
            return;
        }
        bufWork.clear();
        bufCheck.fill(0);
        bufWork.append(file.readAll());
        log(QString("Load from %1 file").arg(fileName));
        log(QString("Read %1 bytes").arg(bufWork.count()));
        if ((uint32_t)bufWork.count() < bufSize) {
            bufWork.append((bufSize - bufWork.count()), 0xff);
        }
        if ((uint32_t)bufWork.count() < bufSize) {
            log(QString("Deleted %1 bytes").arg(bufWork.count() - bufSize));
            bufWork.resize(bufSize);
        }
        bufferClear = false;

        updateButtons(true, true);

        emit bufferUpdated();
    }

}

void MainWindow::on_saveFileButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save buffer"), "",
                                                    tr("Binary (*.bin);;All Files (*)"));

    if (fileName.isEmpty())
        return;
    else {
        if (fileName.right(4).indexOf(".bin", 0) == -1){
            fileName.append(QString(".bin"));
        }
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::information(this, tr("Unable to open file"),
                                     file.errorString());
            return;
        }
        file.write(bufWork);
        file.close();
        log(QString("Buffer saved to file %1").arg(fileName));
    }
}


void MainWindow::verifyData()
{
    QObject::disconnect(verifyDataConnection);
    uint32_t errors_count = 0;
    uint32_t warnings_count = 0;
    for (uint32_t i = 0; i < bufSize; i++){
        // if ((old xor new) and new) err
        if ((bufWork[i] ^ bufCheck[i]) & bufCheck[i]){
            errors_count++;
            bufWork[i] = bufCheck[i];
            // mark error byte
            bufCheck[i] = NOT_WRITABLE;
        } else if (bufWork[i] != bufCheck[i]){
            warnings_count++;
            bufWork[i] = bufCheck[i];
            bufCheck[i] = WRITABLE;
        } else {
            bufCheck[i] = NO_ERR;
        }
        ui->progressBar->setValue(i + 1);
    }
    if (errors_count || warnings_count){
        log(QString("Verification failed."));
        log(QString("Errors: %1.").arg(errors_count));
        log(QString("Warnings: %1.").arg(warnings_count));
    } else {
        log(QString("Verification successful."));
    }
    emit bufferUpdated();
}


void MainWindow::on_showButton_clicked()
{
    bufferDialog->setVisible(true);
    QTableWidget *tableWidget = bufferDialog->tableWidget();
    QTableWidgetItem *newItem;

    // clear table
    tableWidget->setRowCount(0);

    tableWidget->setRowCount((bufSize + 1) / 16);
    for(int row=0; row!=tableWidget->rowCount(); ++row){
        newItem = new QTableWidgetItem(tr("0x%1").arg(QString::asprintf("%05X", (16u * row))));
        tableWidget->setVerticalHeaderItem(row, newItem);
        for(int column=0; column!=tableWidget->columnCount(); ++column) {
            if (16u * row + column < bufSize){
                newItem = new QTableWidgetItem(QString::asprintf("%02X", (uint8_t)bufWork.data()[16u * row + column]));
            }
            else{
                newItem = new QTableWidgetItem("");
            }
            if (bufCheck.data()[16u * row + column] == NOT_WRITABLE) newItem->setForeground(QColor::fromRgb(255,0,0));
            if (bufCheck.data()[16u * row + column] == WRITABLE) newItem->setForeground(QColor::fromRgb(255,127,0));
            tableWidget->setItem(row, column, newItem);
        }
    }
}

void MainWindow::resizeBuffers()
{
    chipSelected = true;

    ui->openFileButton->setEnabled(true);
    ui->readChipButton->setEnabled(true);

    bufWork.resize(bufSize);
    bufCheck.resize(bufSize);
    bufCheck.fill(0);

    emit bufferUpdated();
}


void MainWindow::on_verifyChipButton_clicked()
{
    // Backup work buffer
    bufCheck = bufWork;
    readChip();
    verifyDataConnection = QObject::connect(this, SIGNAL(chipRead()), this, SLOT(verifyData()));
}


void MainWindow::on_c16Button_clicked()
{
    log("Select 27C16 chip");
    bufSize = 0x0800;
    writeData("a");
    emit chipUpdated();
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(bufSize);
}

void MainWindow::on_c32Button_clicked()
{
    log("Select 27C32 chip");
    bufSize = 0x1000;
    writeData("b");
    emit chipUpdated();
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(bufSize);
}

void MainWindow::on_c64Button_clicked()
{
    log("Select 27C64 chip");
    bufSize = 0x2000;
    writeData("c");
    emit chipUpdated();
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(bufSize);
}

void MainWindow::on_c128Button_clicked()
{
    log("Select 27C128 chip");
    bufSize = 0x4000;
    writeData("d");
    emit chipUpdated();
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(bufSize);
}

void MainWindow::on_c256Button_clicked()
{
    log("Select 27C256 chip");
    bufSize = 0x8000;
    writeData("e");
    emit chipUpdated();
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(bufSize);
}

void MainWindow::on_c512Button_clicked()
{
    log("Select 27C512 chip");
    bufSize = 0x10000;
    writeData("f");
    emit chipUpdated();
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(bufSize);
}

void MainWindow::on_c1001Button_clicked()
{
    log("Select 27C1001 chip");
    bufSize = 0x20000;
    writeData("g");
    emit chipUpdated();
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(bufSize);
}

void MainWindow::on_c2001Button_clicked()
{
    log("Select 27C2001 chip");
    bufSize = 0x40000;
    writeData("h");
    emit chipUpdated();
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(bufSize);
}

void MainWindow::on_c4001Button_clicked()
{
    log("Select 27C4001 chip");
    bufSize = 0x80000;
    writeData("i");
    emit chipUpdated();
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(bufSize);
}

void MainWindow::on_c801Button_clicked()
{
    log("Select 27C801 chip");
    bufSize = 0x100000;
    writeData("j");
    emit chipUpdated();
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(bufSize);
}

void MainWindow::showVoltage()
{
    writeData("v");
    QByteArray readData = serialPort->readAll();
    QString str = "Programming voltage: ";
    int8_t pos;
    while (serialPort->waitForReadyRead(80)){
        readData.append(serialPort->readAll());
        if ((pos = readData.indexOf(str, 0)) != -1){
            readData.remove(pos, str.length());
            ui->progressBar->setValue((QString(readData).simplified().toFloat() * 10));
        }
    }
}

void MainWindow::on_voltageChipButton_toggled(bool checked)
{
    if (checked){
        updateVoltageConnection = QObject::connect(&updateVoltageTimer, SIGNAL(timeout()), this, SLOT(showVoltage()));
        updateVoltageTimer.setInterval(300);
        updateVoltageTimer.start();

        // Change progressBar params
        ui->progressBar->setMinimum(50);
        ui->progressBar->setMaximum(300);
        ui->progressBar->setFormat("%vV");
        ui->progressBar->setTextVisible(true);

        showVoltage();
        updateButtons(false, false);
    } else {
        QObject::disconnect(updateVoltageConnection);
        updateVoltageTimer.stop();
        ui->progressBar->setMinimum(0);
        ui->progressBar->setMaximum(100);
        ui->progressBar->setFormat("%p%");
        ui->progressBar->setValue(0);
        updateButtons(true, chipSelected);
    }
}

void MainWindow::on_progressBar_valueChanged(int value)
{
    if (updateVoltageConnection){
         double i = value/10.0;
         QString text;
         text.sprintf("%02.1f", i);
         ui->progressBar->setFormat(text + QString(" V"));
    }
}
