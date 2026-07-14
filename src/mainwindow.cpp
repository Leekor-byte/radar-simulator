#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "RadarCalculator.h"
#include "targetdialog.h"
#include "database.h"

#include <QDoubleValidator>
#include <utility>   // для std::as_const
#include <QIntValidator>
#include <QComboBox>
#include <QPushButton>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_db(nullptr)
{
    ui->setupUi(this);

    // Инициализация базы данных
    m_db = new DatabaseManager(this);
    if (!m_db->initialize()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось инициализировать базу данных");
    }

    // Валидаторы
    ui->leFrequency->setValidator(new QDoubleValidator(9400, 9800, 2, this));
    ui->leAzimuthPoints->setValidator(new QIntValidator(1024, 16384, this));

    // Начальные подсказки (статические)
    ui->leFrequency->setPlaceholderText("9400 – 9800 МГц");
    ui->leGainAntenna->setPlaceholderText("100 – 500 раз");
    ui->leGainRx->setPlaceholderText("5 – 50 дБ");
    ui->leRotationSpeed->setPlaceholderText("10 – 40 об/мин");
    ui->leAzimuthPoints->setPlaceholderText("1024 – 16384");

    // Подключение сигналов для режима, типа сигнала и антенны
    connect(ui->btnCalculate, &QPushButton::clicked,
            this, &MainWindow::onCalculateClicked);
    connect(ui->modeBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onModeChanged);
    connect(ui->signalModeBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onSignalTypeChanged);
    connect(ui->antennaBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onAntennaTypeChanged);

    // Подключение новых кнопок (уже существуют в UI)
    connect(ui->btnDatabase, &QPushButton::clicked,
            this, &MainWindow::onOpenTargetDatabase);
    connect(ui->btnTargetPower, &QPushButton::clicked,
            this, &MainWindow::onCalculatePowerForTarget);

    // Установка начального состояния видимости и подсказок
    onModeChanged(ui->modeBox->currentIndex());
    onSignalTypeChanged(ui->signalModeBox->currentIndex());
    onAntennaTypeChanged(ui->antennaBox->currentIndex());
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::readCommonParams(double &freq, double &lambda,
                                  double &G, double &gainRx_dB,
                                  QString &antennaType, double &beamwidth)
{
    bool ok;
    freq = ui->leFrequency->text().toDouble(&ok);
    if (!ok) return false;
    const double c = 3e8;
    lambda = c / (freq * 1e6);

    G = ui->leGainAntenna->text().toDouble(&ok);
    if (!ok) return false;

    gainRx_dB = ui->leGainRx->text().toDouble(&ok);
    if (!ok) return false;

    antennaType = ui->antennaBox->currentText();
    beamwidth = ui->leBeamwidth->text().toDouble(&ok);
    if (!ok) return false;

    if (freq < 9400 || freq > 9800) return false;
    if (G < 100 || G > 500) return false;
    if (gainRx_dB < 5 || gainRx_dB > 50) return false;
    if (antennaType == "Щелевая") {
        if (beamwidth < 0.2 || beamwidth > 4.0) return false;
    } else {
        if (beamwidth < 3.0 || beamwidth > 6.0) return false;
    }
    return true;
}

void MainWindow::showResult(const QString &text) {
    ui->labelResult->setText(text);
}

void MainWindow::onModeChanged(int index) {
    QString mode = ui->modeBox->currentText();
    bool isTime = (mode == "Временной анализ");
    bool isPower = (mode == "Мощностной анализ");
    bool isCombined = (mode == "Комбинированный анализ");

    // Всегда видимые контейнеры
    ui->containerFreq->setVisible(true);
    ui->containerGainAntenna->setVisible(true);
    ui->containerGainRx->setVisible(true);
    ui->containerBeamwidth->setVisible(true);
    ui->containerTau->setVisible(true);

    // Мощность и период — в мощностном и комбинированном
    bool showPower = isPower || isCombined;
    ui->containerPower->setVisible(showPower);
    ui->containerPeriod->setVisible(showPower);

    // Скорость вращения и точки — во временном и комбинированном
    bool showTime = isTime || isCombined;
    ui->containerRPM->setVisible(showTime);
    ui->containerAzimuth->setVisible(showTime);
}

void MainWindow::onSignalTypeChanged(int index) {
    QString signalType = ui->signalModeBox->currentText();
    if (signalType == "Импульсный") {
        ui->leTau->setPlaceholderText("0.05 – 1 мкс");
        ui->lePeriod->setPlaceholderText("340 – 2000 мкс");
        ui->lePt->setPlaceholderText("5000 – 60000 Вт");
    } else {
        ui->leTau->setPlaceholderText("0.05 – 30 мкс");
        ui->lePeriod->setPlaceholderText("10 – 200 мкс");
        ui->lePt->setPlaceholderText("1 – 200 Вт");
    }
}

void MainWindow::onAntennaTypeChanged(int index) {
    QString antenna = ui->antennaBox->currentText();
    if (antenna == "Щелевая") {
        ui->leBeamwidth->setPlaceholderText("0.2 – 4°");
    } else {
        ui->leBeamwidth->setPlaceholderText("3 – 6°");
    }
}

void MainWindow::onCalculateClicked()
{
    QString mode = ui->modeBox->currentText();
    QString signalType = ui->signalModeBox->currentText();

    double freq, lambda, G, gainRx_dB, beamwidth;
    QString antennaType;
    if (!readCommonParams(freq, lambda, G, gainRx_dB, antennaType, beamwidth)) {
        showResult("Ошибка: неверные общие параметры");
        return;
    }

    double rpm = ui->leRotationSpeed->text().toDouble();
    int azimuthPoints = ui->leAzimuthPoints->text().toInt();
    double tau = ui->leTau->text().toDouble();
    double period = ui->lePeriod->text().toDouble();
    double Pt = ui->lePt->text().toDouble();

    QString result;

    if (mode == "Временной анализ") {
        if (rpm <= 0 || azimuthPoints <= 0 || tau <= 0) {
            showResult("Ошибка: неверные входные данные для временного анализа");
            return;
        }
        auto res = RadarCalculator::performTimeAnalysis(rpm, azimuthPoints, tau, beamwidth, signalType);
        if (res.feasible) {
            result = QString("Временной анализ: РЕЖИМ ВОЗМОЖЕН\n"
                             "Максимальная дальность: %1 м\n"
                             "Период импульсов: %2 мкс")
                         .arg(res.maxRange)
                         .arg(res.pulsePeriod * 1e6);
        } else {
            result = "Временной анализ: НЕВОЗМОЖЕН\n" + res.message;
        }
    }
    else if (mode == "Мощностной анализ") {
        if (tau <= 0 || period <= 0 || Pt <= 0) {
            showResult("Ошибка: неверные входные данные для мощностного анализа");
            return;
        }
        auto res = RadarCalculator::performPowerAnalysis(tau, period, Pt, G, lambda, gainRx_dB, signalType);
        if (res.maxRange > 0) {
            result = QString("Мощностной анализ:\n"
                             "Максимальная дальность для цели с ЭПР 10 м²: %1 м").arg(res.maxRange);
        } else {
            result = "Мощностной анализ: неверные параметры";
        }
    }
    else if (mode == "Комбинированный анализ") {
        if (rpm <= 0 || azimuthPoints <= 0 || tau <= 0 || period <= 0 || Pt <= 0) {
            showResult("Ошибка: неверные входные данные для комбинированного анализа");
            return;
        }
        auto res = RadarCalculator::performCombinedAnalysis(rpm, azimuthPoints, tau, period,
                                                            beamwidth, Pt, G, lambda, gainRx_dB, signalType);
        result = QString("Комбинированный анализ:\n"
                         "Дальность по времени: %1 м\n"
                         "Дальность по мощности: %2 м\n"
                         "Количество импульсов в пачке: %3\n\n"
                         "%4")
                     .arg(res.timeRange)
                     .arg(res.powerRange)
                     .arg(res.pulseCount)
                     .arg(res.conclusion);
    }
    showResult(result);
}

// ---------- НОВЫЕ СЛОТЫ ----------

void MainWindow::onOpenTargetDatabase()
{
    TargetDialog dlg(m_db, this);
    dlg.exec();
}

void MainWindow::onCalculatePowerForTarget()
{
    auto targets = m_db->allTargets();
    if (targets.isEmpty()) {
        showResult("Нет целей в базе данных");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Расчёт мощности по цели");
    QFormLayout form(&dlg);

    QComboBox combo;
    for (const auto &t : std::as_const(targets)) {
        combo.addItem(QString("%1 (%2, %3 м²)").arg(t.name, t.type).arg(t.rcs), t.id);
    }

    QLineEdit rangeEdit;
    rangeEdit.setPlaceholderText("Дальность (м)");
    form.addRow("Цель", &combo);
    form.addRow("Дальность", &rangeEdit);

    QPushButton calcBtn("Рассчитать");
    form.addRow(&calcBtn);

    QLabel resultLabel;
    resultLabel.setWordWrap(true);
    form.addRow("Результат", &resultLabel);

    QObject::connect(&calcBtn, &QPushButton::clicked, [&]() {
        bool ok;
        double R = rangeEdit.text().toDouble(&ok);
        if (!ok || R <= 0) {
            resultLabel.setText("Некорректная дальность");
            return;
        }
        int id = combo.currentData().toInt();
        double sigma = 0;
        for (const auto &t : std::as_const(targets)) {
            if (t.id == id) { sigma = t.rcs; break; }
        }

        double freq = ui->leFrequency->text().toDouble();
        double G = ui->leGainAntenna->text().toDouble();
        double Pt = ui->lePt->text().toDouble();
        if (freq <= 0 || G <= 0 || Pt <= 0) {
            resultLabel.setText("Не заданы параметры радара (частота, усиление, мощность)");
            return;
        }
        double lambda = 3e8 / (freq * 1e6);
        double Pr = RadarCalculator::calculateReceivedPower(Pt, G, G, lambda, sigma, R);
        resultLabel.setText(QString("Принимаемая мощность: %1 Вт").arg(Pr, 0, 'e', 6));
    });

    dlg.exec();
}
