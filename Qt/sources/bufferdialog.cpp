#include "bufferdialog.h"
#include "ui_bufferdialog.h"

BufferDialog::BufferDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BufferDialog)
{
    ui->setupUi(this);
}

BufferDialog::~BufferDialog()
{
    delete ui;
}

QTableWidget* BufferDialog::tableWidget()
{
    return ui->tableWidget;
}
