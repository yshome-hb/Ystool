#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTimer>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QTextEdit>
#include <QSerialPort>
#include <QPushButton>
#include <QMainWindow>
#include <QTextBrowser>
#include <QSerialPortInfo>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 串口参数上下文
    struct SerialPortParametersContext {
        QString name;
        quint32 baudRate;
        quint32 stopBits;
        quint32 dataBits;
        quint32 parity;
        quint32 control; // 流控标志
    };

    // 输入格式
    enum InputFromat {
        InputHex,
        InputAscii,
        InputSystem
    };
    Q_ENUM(InputFromat);

    // 输出格式
    enum OutputFromat {
        OutputHex,
        OutputUtf8,
        OutputAscii,
        OutputSystem
    };
    Q_ENUM(OutputFromat);
private:
    SerialPortParametersContext parametersContext;
    InputFromat outputFormat;
    InputFromat inputFormat;
    QSerialPort serialPort;
    QTimer writeBytesTimer;
private:
    void initDevicesComboBox();
    void initBaudrateComboBox();
    void initStopbitsComboBox();
    void initDatabitsComboBox();
    void initParityComboBox();
    void initControlComboBox();

    void setUiEnable(bool opened);
    void readBytes();
    QString cookOutputData(QByteArray rawData, int format, bool tx);
    QByteArray cookInputData(QString rawData, int format);
    void formattingInputText();
private:
    Ui::MainWindow *ui;
    QComboBox *devicesComboBox;
    QComboBox *baudrateComboBox;
    QComboBox *databitsComboBox;
    QComboBox *controlComboBox;
    QComboBox *parityComboBox;
    QComboBox *stopbitsComboBox;
    QPushButton *refreshPushButton;
    QPushButton *openPushButton;
    QComboBox *outputFormatComboBox;
    QComboBox *inputFormatComboBox;
    QCheckBox *checkBox;
    QLineEdit *lineEdit;
    QPushButton *sendPushButton;
    QTextBrowser *textBrowser;
    QTextEdit *textEdit;
private slots:
    void on_refreshPushButton_clicked();
    void on_openPushButton_clicked();
    void on_sendPushButton_clicked();
    void on_devicesComboBox_currentTextChanged(const QString &arg1);
    void on_databitsComboBox_currentTextChanged(const QString &arg1);
    void on_stopBitsComboBox_currentTextChanged(const QString &arg1);
    void on_parityComboBox_currentTextChanged(const QString &arg1);
    void on_controlComboBox_currentTextChanged(const QString &arg1);
    void on_baudrateComboBox_currentTextChanged(const QString &arg1);
    void on_inputFormatComboBox_currentTextChanged(const QString &arg1);
    void on_outputFormatComboBox_currentTextChanged(const QString &arg1);
    void on_checkBox_clicked();
};
#endif // MAINWINDOW_H
