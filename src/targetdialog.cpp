#include "targetdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <cmath>

TargetDialog::TargetDialog(DatabaseManager *db, QWidget *parent)
    : QDialog(parent), m_db(db)
{
    setWindowTitle("База целей");
    setupUI();
    refreshTable();
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int idx) {
                bool isShip = (m_typeCombo->currentText() == "Судно");
                m_displacementEdit->setVisible(isShip);
                m_displacementLabel->setVisible(isShip);
                m_rcsEdit->setVisible(!isShip);
                m_rcsLabel->setVisible(!isShip);
            });
    // начальное состояние
    m_typeCombo->currentIndexChanged(0);
}

void TargetDialog::setupUI()
{
    auto mainLayout = new QVBoxLayout(this);

    // Таблица
    m_table = new QTableWidget(0, 4);
    m_table->setHorizontalHeaderLabels({"ID", "Название", "Тип", "ЭПР (м²)"});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_table);

    // Форма добавления
    auto formGroup = new QGroupBox("Добавить цель");
    auto formLayout = new QFormLayout(formGroup);

    m_nameEdit = new QLineEdit;
    m_typeCombo = new QComboBox;
    m_typeCombo->addItems({"Судно", "Ориентир"});

    m_displacementEdit = new QLineEdit;
    m_displacementEdit->setPlaceholderText("тыс. тонн");
    m_displacementLabel = new QLabel("Водоизмещение (тыс. т)");

    m_rcsEdit = new QLineEdit;
    m_rcsEdit->setPlaceholderText("м²");
    m_rcsLabel = new QLabel("ЭПР (м²)");

    formLayout->addRow("Название", m_nameEdit);
    formLayout->addRow("Тип", m_typeCombo);
    formLayout->addRow(m_displacementLabel, m_displacementEdit);
    formLayout->addRow(m_rcsLabel, m_rcsEdit);

    mainLayout->addWidget(formGroup);

    // Кнопки
    auto btnLayout = new QHBoxLayout;
    auto addBtn = new QPushButton("Добавить");
    auto delBtn = new QPushButton("Удалить выбранное");
    btnLayout->addStretch();
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(delBtn);
    mainLayout->addLayout(btnLayout);

    connect(addBtn, &QPushButton::clicked, this, &TargetDialog::onAddTarget);
    connect(delBtn, &QPushButton::clicked, this, &TargetDialog::onDeleteTarget);
}

void TargetDialog::refreshTable()
{
    auto targets = m_db->allTargets();
    m_table->setRowCount(targets.size());
    for (int i = 0; i < targets.size(); ++i) {
        const auto &t = targets[i];
        m_table->setItem(i, 0, new QTableWidgetItem(QString::number(t.id)));
        m_table->setItem(i, 1, new QTableWidgetItem(t.name));
        m_table->setItem(i, 2, new QTableWidgetItem(t.type));
        m_table->setItem(i, 3, new QTableWidgetItem(QString::number(t.rcs, 'f', 2)));
    }
}

void TargetDialog::onAddTarget()
{
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите название цели");
        return;
    }
    TargetInfo t;
    t.name = name;
    t.type = m_typeCombo->currentText();

    if (t.type == "Судно") {
        bool ok;
        double d = m_displacementEdit->text().toDouble(&ok);
        if (!ok || d <= 0) {
            QMessageBox::warning(this, "Ошибка", "Некорректное водоизмещение");
            return;
        }
        t.displacement = d;
        t.rcs = 52.0 * pow(d, 2.0/3.0);
    } else { // Ориентир
        bool ok;
        double rcs = m_rcsEdit->text().toDouble(&ok);
        if (!ok || rcs <= 0) {
            QMessageBox::warning(this, "Ошибка", "Некорректная ЭПР");
            return;
        }
        t.rcs = rcs;
        t.displacement = 0;
    }

    if (m_db->addTarget(t)) {
        m_nameEdit->clear();
        m_displacementEdit->clear();
        m_rcsEdit->clear();
        refreshTable();
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось добавить цель в БД");
    }
}

void TargetDialog::onDeleteTarget()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Информация", "Выберите цель для удаления");
        return;
    }
    int id = m_table->item(row, 0)->text().toInt();
    if (m_db->removeTarget(id)) {
        refreshTable();
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось удалить цель");
    }
}
