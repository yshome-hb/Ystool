#include <QDebug>
#include <QMetaEnum>
#include <QDateTime>
#include <QMessageBox>
#include <QStandardItem>
#include <QStandardItemModel>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    devicesComboBox = ui->devicesComboBox;
    baudrateComboBox = ui->baudrateComboBox;
    databitsComboBox = ui->databitsComboBox;
    controlComboBox = ui->controlComboBox;
    parityComboBox = ui->parityComboBox;
    stopbitsComboBox = ui->stopBitsComboBox;
    refreshPushButton = ui->refreshPushButton;
    openPushButton = ui->openPushButton;
    outputFormatComboBox = ui->outputFormatComboBox;
    inputFormatComboBox = ui->inputFormatComboBox;
    checkBox = ui->checkBox;
    checkBox->setEnabled(false);
    lineEdit = ui->lineEdit;
    sendPushButton = ui->sendPushButton;
    textBrowser = ui->textBrowser;
    textEdit = ui->textEdit;

    // 将串口信息添加至相应的控件中
    on_refreshPushButton_clicked();

    // 初始化输入/输出格式选择框
    QMetaEnum parityEnum = QMetaEnum::fromType<MainWindow::InputFromat>();
    for (int i = 0; i < parityEnum.keyCount(); i++){
        inputFormatComboBox->addItem(parityEnum.key(i), QVariant::fromValue(parityEnum.value(i)));
    }
    inputFormatComboBox->setCurrentIndex(1);

    parityEnum = QMetaEnum::fromType<MainWindow::OutputFromat>();
    for (int i = 0; i < parityEnum.keyCount(); i++){
        outputFormatComboBox->addItem(parityEnum.key(i), QVariant::fromValue(parityEnum.value(i)));
    }
    outputFormatComboBox->setCurrentIndex(3);

    // 收到读就绪信息后，读取串口数据
    connect(&serialPort, &QSerialPort::readyRead, this, &MainWindow::readBytes);
    // 输入框数据发生改变时，格式化输入数据
    connect(textEdit, &QTextEdit::textChanged, this, &MainWindow::formattingInputText);
    // 定时发送数据
    connect(&writeBytesTimer, &QTimer::timeout, this, &MainWindow::on_sendPushButton_clicked);
}

MainWindow::~MainWindow()
{
    delete ui;
    if (writeBytesTimer.isActive()){
        writeBytesTimer.stop();
    }

    // 在Linux平台，串口打开后直接关闭软件，串口不会关闭，会导致再次打开程序后无法打开串口
    // Windows则不存在该问题，此处在软件退出前关闭串口是比较稳妥的做法
    if (serialPort.isOpen()){
        serialPort.close();
    }
}

void MainWindow::initDevicesComboBox()
{
    // 获取本机所有串口设备信息
    QList<QSerialPortInfo> infoList = QSerialPortInfo::availablePorts();

    // 为了在下拉选项中显示更多信息，采用模型形式添加设备列表
    QStandardItemModel *model = new QStandardItemModel(devicesComboBox);
    for (auto var : infoList){
        QStandardItem *item = new QStandardItem(var.portName());
        item->setToolTip(var.portName() + " " + var.description());
        model->appendRow(item);
    }

    // 将设备信息添加至设备选择框中，添加前清除原有信息
    devicesComboBox->clear();
    devicesComboBox->setModel(model);
}

void MainWindow::initBaudrateComboBox()
{
    baudrateComboBox->clear();
    QList<qint32> bdList = QSerialPortInfo::standardBaudRates();
    for (auto var : bdList){
        baudrateComboBox->addItem(QString::number(var), QVariant::fromValue(var));
    }
    baudrateComboBox->setCurrentText("9600");
}

void MainWindow::initStopbitsComboBox()
{
    stopbitsComboBox->clear();
    QMetaEnum stopBitsEnum = QMetaEnum::fromType<QSerialPort::StopBits>();
    for (int i = 0; i < stopBitsEnum.keyCount(); i++){
        if (stopBitsEnum.value(i) != QSerialPort::UnknownStopBits){
            stopbitsComboBox->addItem(stopBitsEnum.key(i), QVariant::fromValue(stopBitsEnum.value(i)));
        }
    }
}

void MainWindow::initDatabitsComboBox()
{
    databitsComboBox->clear();
    QMetaEnum dataBitsEnum = QMetaEnum::fromType<QSerialPort::DataBits>();
    int defaultIndex = 0;
    for (int i = 0; i < dataBitsEnum.keyCount(); i++){
        if (dataBitsEnum.value(i) != QSerialPort::UnknownDataBits){
            databitsComboBox->addItem(dataBitsEnum.key(i), QVariant::fromValue(dataBitsEnum.value(i)));
            if (dataBitsEnum.value(i) == QSerialPort::Data8){
                defaultIndex = i;
            }
        }
    }

    databitsComboBox->setCurrentIndex(defaultIndex);
}

void MainWindow::initParityComboBox()
{
    parityComboBox->clear();
    QMetaEnum parityEnum = QMetaEnum::fromType<QSerialPort::Parity>();
    for (int i = 0; i < parityEnum.keyCount(); i++){
        if (parityEnum.value(i) != QSerialPort::UnknownParity){
            parityComboBox->addItem(parityEnum.key(i), QVariant::fromValue(parityEnum.value(i)));
        }
    }
}

void MainWindow::initControlComboBox()
{
    controlComboBox->clear();
    QMetaEnum flowEnum = QMetaEnum::fromType<QSerialPort::FlowControl>();
    for (int i = 0; i < flowEnum.keyCount(); i++){
        if (flowEnum.value(i) != QSerialPort::UnknownFlowControl){
            controlComboBox->addItem(flowEnum.key(i), QVariant::fromValue(flowEnum.value(i)));
        }
    }
}

void MainWindow::setUiEnable(bool opened)
{
    devicesComboBox->setEnabled(!opened);
    baudrateComboBox->setEnabled(!opened);
    databitsComboBox->setEnabled(!opened);
    controlComboBox->setEnabled(!opened);
    parityComboBox->setEnabled(!opened);
    stopbitsComboBox->setEnabled(!opened);
    refreshPushButton->setEnabled(!opened);
    checkBox->setEnabled(opened);
}

void MainWindow::readBytes()
{
    QByteArray bytes = serialPort.readAll();

    // 读取数据后，将数据输出至ui
    textBrowser->append(cookOutputData(bytes, outputFormat, false));
}

QString MainWindow::cookOutputData(QByteArray rawData, int format, bool tx)
{
    QString outputString = QString("[%1 %2]")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
            .arg(tx ? QString("<font color=green>Tx</font>") : QString("<font color=purple>Rx</font>"));
    outputString = QString("<font color=silver>%2<font>").arg(outputString);

    QString dataString;
    if (format == MainWindow::OutputHex){
        dataString = rawData.toHex(' ');
    }else if (format == MainWindow::OutputUtf8){
        dataString = QString::fromUtf8(rawData);
    }else if (format == MainWindow::OutputAscii){
        dataString = QString::fromLatin1(rawData);
    }else if(format == MainWindow::OutputSystem){
        dataString = QString::fromLocal8Bit(rawData);
    }
    dataString = QString("<font color=black>%2<font>").arg(dataString);

    outputString.append(dataString);
    return outputString;
}

QByteArray MainWindow::cookInputData(QString rawData, int format)
{
    QByteArray inputData;
    if (format == MainWindow::InputHex){
        rawData = rawData.trimmed();
        QStringList stringList = rawData.split(' ');
        for (auto var : stringList){
            if (var.length()){
                quint8 byte = var.toInt(Q_NULLPTR, 16);
                inputData.append(byte);
            }
        }
    }else if(format == MainWindow::InputAscii){
        inputData = rawData.toLatin1();
    }else if(format == MainWindow::InputSystem){
        inputData = rawData.toLocal8Bit();
    }

    return inputData;
}

void MainWindow::formattingInputText()
{
    textEdit->blockSignals(true);

    QString plaintext = textEdit->toPlainText();
    if (!plaintext.isEmpty()){
        if(inputFormat == MainWindow::InputHex) {
            QString strTemp;
            plaintext.remove(QRegExp("[^0-9a-fA-F]"));
            for (int i = 0; i < plaintext.length(); i++){
                if ((i != 0) && (i % 2 == 0)){
                    strTemp.append(QChar(' '));
                }
                strTemp.append(plaintext.at(i));
            }
            textEdit->setText(strTemp.toUpper());
            textEdit->moveCursor(QTextCursor::End);
        }else if(inputFormat == MainWindow::InputAscii) {
            QString newString;
            for (int i = 0; i < plaintext.count(); i++){
                if (plaintext.at(i).unicode() <= 127){
                    newString.append(plaintext.at(i));
                }
            }
            textEdit->setText(newString);
            textEdit->moveCursor(QTextCursor::End);
        }
    }

    textEdit->blockSignals(false);
}

void MainWindow::on_refreshPushButton_clicked()
{
    initDevicesComboBox();
    initBaudrateComboBox();
    initStopbitsComboBox();
    initDatabitsComboBox();
    initParityComboBox();
    initControlComboBox();
}

void MainWindow::on_openPushButton_clicked()
{
    if (serialPort.isOpen()){
        writeBytesTimer.stop();
        serialPort.close();
        openPushButton->setText(tr("打开"));
        setUiEnable(false);
    }else{
        serialPort.setParity(static_cast<QSerialPort::Parity>(parametersContext.parity));
        serialPort.setBaudRate(parametersContext.baudRate);
        serialPort.setDataBits(static_cast<QSerialPort::DataBits>(parametersContext.dataBits));
        serialPort.setPortName(parametersContext.name);
        serialPort.setStopBits(static_cast<QSerialPort::StopBits>(parametersContext.stopBits));
        serialPort.setFlowControl(static_cast<QSerialPort::FlowControl>(parametersContext.control));

        if (serialPort.open(QSerialPort::ReadWrite)){
            openPushButton->setText(tr("关闭"));
            setUiEnable(true);
        }else{
            QMessageBox::warning(this, tr("遇到错误"), serialPort.errorString());
        }
    }
}

void MainWindow::on_sendPushButton_clicked()
{
    if (!serialPort.isOpen()){
        QMessageBox::warning(this, tr("设备错误"), tr("请打开设备后尝试"));
        return;
    }

    // 如果输入框为空，则发送"empty"，输入数据会按照输入格式进行处理，处理后的数据数据为字节数组
    QString rawData = textEdit->toPlainText().length() ? textEdit->toPlainText() : QString("empty");
    QByteArray data = cookInputData(rawData, inputFormat);
    if (serialPort.write(data)){
        QString outputText = cookOutputData(data, outputFormat, true);
        textBrowser->append(outputText);
    }
}

void MainWindow::on_devicesComboBox_currentTextChanged(const QString &arg1)
{
    parametersContext.name = arg1;
}

void MainWindow::on_databitsComboBox_currentTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    parametersContext.dataBits = databitsComboBox->currentData().toInt();
}

void MainWindow::on_stopBitsComboBox_currentTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    parametersContext.stopBits = stopbitsComboBox->currentData().toInt();
}

void MainWindow::on_parityComboBox_currentTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    parametersContext.parity = parityComboBox->currentData().toInt();
}

void MainWindow::on_controlComboBox_currentTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    parametersContext.control = controlComboBox->currentData().toInt();
}


void MainWindow::on_baudrateComboBox_currentTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    parametersContext.baudRate = baudrateComboBox->currentData().toInt();
}

void MainWindow::on_inputFormatComboBox_currentTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    textEdit->clear();
    inputFormat = static_cast<MainWindow::InputFromat>(inputFormatComboBox->currentData().toInt());
}

void MainWindow::on_outputFormatComboBox_currentTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    outputFormat = static_cast<MainWindow::InputFromat>(outputFormatComboBox->currentData().toInt());
}

void MainWindow::on_checkBox_clicked()
{
    if (checkBox->isChecked()){
        int interval = lineEdit->text().toInt();
        if (interval < 10){
            interval = 10;
        }

        writeBytesTimer.setInterval(interval);
        writeBytesTimer.start();
        lineEdit->setEnabled(false);
    }else{
        writeBytesTimer.stop();
        lineEdit->setEnabled(true);
    }
}
