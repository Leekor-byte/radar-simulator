#ifndef TARGETDIALOG_H
#define TARGETDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include "database.h"
#include <QLabel>

class TargetDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TargetDialog(DatabaseManager *db, QWidget *parent = nullptr);

private slots:
    void onAddTarget();
    void onDeleteTarget();

private:
    void setupUI();
    void refreshTable();

    DatabaseManager *m_db;
    QTableWidget *m_table;
    QLineEdit *m_nameEdit;
    QComboBox *m_typeCombo;
    QLineEdit *m_displacementEdit;  // только для судна
    QLineEdit *m_rcsEdit;           // для ориентира
    QLabel *m_displacementLabel;
    QLabel *m_rcsLabel;
};
#endif // TARGETDIALOG_H
