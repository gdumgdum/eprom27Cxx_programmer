#ifndef BUFFERDIALOG_H
#define BUFFERDIALOG_H

#include <QDialog>
#include <QTableWidget>

namespace Ui {
class BufferDialog;
}

class BufferDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BufferDialog(QWidget *parent = nullptr);
    ~BufferDialog();

    QTableWidget* tableWidget();

private:
    Ui::BufferDialog *ui;
};

#endif // BUFFERDIALOG_H
