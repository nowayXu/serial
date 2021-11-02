#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QMessageBox>
#include <QDebug>
#include <regex>

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QScatterSeries>
#include <QtWidgets/QLabel>
#include <QtCore/QRandomGenerator>
#include <QtCharts/QValueAxis>

#include <QTimer>


using namespace std;

QSerialPort *m_serialPort = new QSerialPort();//实例化串口类一个对象
QStringList m_serialPortName;
QStringList m_serialPortInfo;

//这样我们就获取到 可用的串口名字了





//构造 函数
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_listCount(3),
      m_valueMax(10),
      m_valueCount(7),
      m_dataTable(generateRandomData(m_listCount, m_valueMax, m_valueCount)),

      ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    SearchCom();

    //  气囊尺寸 计数  初始化
    inAirCut = 0;
    outAirCut = 0;
    setCut = 0;
    airSize = 0;


    Buling_Flag = true;
    BulingBuling_interval = 500;
    BulingBuling = new QTimer();
    BulingBuling->setInterval(BulingBuling_interval);
    connect(BulingBuling,SIGNAL(timeout()),this,SLOT(Buling_timeout()));
    BulingBuling->start(BulingBuling_interval);

    //充气放气 时间 初始化
//    InTime="500";
//    ui->InAirTime_lcdNumber->display(InTime);



//    OutTime="500";
//    ui->OutAirTime_lcdNumber->display(OutTime);



    //create charts
    QChartView *chartView;
    chartView = new QChartView(createSplineChart());
    ui->gridLayout_charts->addWidget(chartView);
    m_charts << chartView;




}



//析构函数
MainWindow::~MainWindow()
{
    delete ui;
}



//搜索串口

void MainWindow::SearchCom(){

    m_serialPortName.clear();
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        m_serialPortName << info.portName();
        m_serialPortInfo << "(" + info.portName() + ")" + info.description();

    }
    if(m_serialPortName.size() != 0){
        ui->comboBox_com->clear();
        ui->comboBox_com->addItems(m_serialPortInfo);

    }
    else{
        QMessageBox::warning(this,"信息","找不到可用的串口信息！！");
    }



}

void MainWindow::OpenCom(){

    if(m_serialPort->isOpen())//如果串口已经打开了 先给他关闭了
    {
        m_serialPort->clear();
        m_serialPort->close();
    }
    else{
        QString portCom = ui->comboBox_com->currentText();

        string strPortCom = portCom.toStdString();
        regex pattern("\\((.*)\\)");  //匹配格式，C++11切记注'\\'代表转义'\' 非常重要！！！
        smatch result;  //暂存匹配结果

        //迭代器声
        string::const_iterator iterStart = strPortCom.begin();
        string::const_iterator iterEnd = strPortCom.end();

        //提取COM口字符串
        if(regex_search(iterStart, iterEnd, result, pattern))
        {
            cout << result.str(1)<<endl;
            portCom = QString::fromStdString(result.str(1));
            m_serialPort->setPortName(portCom);

            if(!m_serialPort->open(QIODevice::ReadWrite))//用ReadWrite 的模式尝试打开串口
            {
                QMessageBox::warning(this,"信息","串口打开失败！！");
                ui->radioButton_switchCom->setChecked(false);
                return;
            }
            else{
                ui->radioButton_switchCom->setText("串口已打开！");
                ui->radioButton_switchCom->setChecked(true);
            }
        }
    }


    m_serialPort->setBaudRate(ui->comboBox_baudrate->currentText().toInt(),QSerialPort::AllDirections);//设置波特率和读写方向
    m_serialPort->setDataBits(QSerialPort::Data8);		//数据位为8位
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);//无流控制
    m_serialPort->setParity(QSerialPort::NoParity);	//无校验位
    m_serialPort->setStopBits(QSerialPort::OneStop); //一位停止位

    //连接信号槽 当下位机发送数据QSerialPortInfo 会发送个 readyRead 信号,我们定义个槽void receiveInfo()解析数据
    connect(m_serialPort,SIGNAL(readyRead()),this,SLOT(ReceiveInfo()));

}



void MainWindow::on_pushButton_scanCom_clicked()
{
    SearchCom();
}


void MainWindow::on_radioButton_switchCom_clicked()
{
    if(ui->radioButton_switchCom->isChecked()){

        OpenCom();

//        QByteArray orderInfo_In = InTime.toLatin1();
//        orderInfo_In  += '#';
//        SendInfo(orderInfo_In);


//        QByteArray orderInfo_Out = OutTime.toLatin1();
//        orderInfo_Out  += '*';
//        SendInfo(orderInfo_Out);




    }
    else{
        CloseCom();
    }

}


void MainWindow::CloseCom(){
    ui->radioButton_switchCom->setChecked(false);
    if(m_serialPort->isOpen())
    {
        m_serialPort->clear();
        m_serialPort->close();
    }
    ui->radioButton_switchCom->setText("串口未打开！");
}



//发送函数
void MainWindow::SendInfo(QByteArray &info)
{
    if(m_serialPort->isOpen())
    {
        qDebug()<<"Write to serial: "<< info ;
        m_serialPort->write(info);//这句是真正的给单片机发数据 用到的是QIODevice::write 具体可以看文档
    }
    else
    {
        QMessageBox::warning(this,"信息","串口还未打开哎！！");
    }
}


//接收 函数
void MainWindow::ReceiveInfo()
{
    QByteArray info = m_serialPort->readAll();
    qDebug()<< "read  is = "<< info ;
    ui->textBrowser_receive->setText(info);
    if(info == "#"){
        inAirCut++;
        ui->lcdNumber_inCut->display(inAirCut);
        ui->lcdNumber_size->display(setCut+inAirCut-outAirCut);
        ui->frame->setStyleSheet("background-color: rgb(255, 255, 127);border-radius: 7px;");
    }
    else if(info == "*" && (setCut+inAirCut-outAirCut-1>=0))
    {
        outAirCut++;
        ui->lcdNumber_outCut->display(outAirCut);
        ui->lcdNumber_size->display(setCut+inAirCut-outAirCut);
        ui->frame->setStyleSheet("background-color: rgb(255, 255, 127);border-radius: 7px;");
    }
}



//绘图
DataTable MainWindow::generateRandomData(int listCount, int valueMax, int valueCount) const
{
    DataTable dataTable;

    // generate random data
    for (int i(0); i < listCount; i++) {
        DataList dataList;
        qreal yValue(0);
        for (int j(0); j < valueCount; j++) {
            yValue = yValue + QRandomGenerator::global()->bounded(valueMax / (qreal) valueCount);
            QPointF value((j + QRandomGenerator::global()->generateDouble()) * ((qreal) m_valueMax / (qreal) valueCount),
                          yValue);
            QString label = "Slice " + QString::number(i) + ":" + QString::number(j);
            dataList << Data(value, label);
        }
        dataTable << dataList;
    }

    return dataTable;
}


QChart *MainWindow::createSplineChart() const
{
    QChart *chart = new QChart();
    chart->setTitle("Spline chart");
    QString name("Series ");
    int nameIndex = 0;
    for (const DataList &list : m_dataTable) {
        QSplineSeries *series = new QSplineSeries(chart);
        for (const Data &data : list)
            series->append(data.first);
        series->setName(name + QString::number(nameIndex));
        nameIndex++;
        chart->addSeries(series);
    }

    chart->createDefaultAxes();
    chart->axes(Qt::Horizontal).first()->setRange(0, m_valueMax);
    chart->axes(Qt::Vertical).first()->setRange(0, m_valueCount);

    // Add space to label to add space between labels and axis
    QValueAxis *axisY = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).first());
    Q_ASSERT(axisY);
    axisY->setLabelFormat("%.1f  ");
    return chart;
}





void MainWindow::on_comboBox_baudrate_currentTextChanged(const QString &arg1)
{
    CloseCom();
}



//充气
void MainWindow::on_pushButton_inAir_clicked()
{
    QByteArray orderInfo = "1@";
    SendInfo(orderInfo);
    ui->frame->setStyleSheet("background-color: rgb(0, 255, 0);border-radius: 7px;");
}

//放气
void MainWindow::on_pushButton_outAir_clicked()
{
    QByteArray orderInfo = "2@";

    if(setCut+inAirCut-outAirCut-1>=0 && orderInfo=="2@")
    {
      SendInfo(orderInfo);
      ui->frame->setStyleSheet("background-color: rgb(255, 0, 0);border-radius: 7px;");
    }

}

//清零操作
void MainWindow::on_pushButton_SetZero_clicked()
{
    setCut += inAirCut - outAirCut;
    inAirCut = 0;
    outAirCut = 0;
    ui->lcdNumber_inCut->display(inAirCut);
    ui->lcdNumber_outCut->display(outAirCut);

}



//设置充气时间
void MainWindow::on_pushButton_InAirTimeSet_clicked()
{



   InTime = ui->InAir_lineEdit->text();
   ui->InAirTime_lcdNumber->display(InTime);

   QByteArray orderInfo = InTime.toLatin1();
   orderInfo  += '#';
   SendInfo(orderInfo);
   qDebug()<<"From In Write to serial to set InTime: "<< orderInfo ;

}

//设置放气时间
void MainWindow::on_pushButton_OutAirTimeSet_clicked()
{
    OutTime = ui->OutAir_lineEdit->text();
    ui->OutAirTime_lcdNumber->display(OutTime);

    QByteArray orderInfo = OutTime.toLatin1();
    orderInfo  += '*';
    SendInfo(orderInfo);
    qDebug()<<"From Out Write to serial to set OutTime: "<< orderInfo ;
}

void MainWindow::Buling_timeout()
{
    //控制  灯
    if(Buling_Flag)
    {
     ui->frame_Buling->setStyleSheet("background-color: rgb(255, 255, 255);border-radius:7px;");
    }
    else
    {
    ui->frame_Buling->setStyleSheet("background-color: rgb(255, 0, 127);border-radius:7px;");
    }
    Buling_Flag=!Buling_Flag;
}



void MainWindow::on_pushButton_clicked()
{
    //一点 这个按钮，就停止、开始闪烁
    //disconnect(m_serialPort,SIGNAL(readyRead()),this,SLOT(ReceiveInfo()));
}

