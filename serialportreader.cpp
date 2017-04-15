/****************************************************************************
**
** Copyright (C) 2013 Laszlo Papp <lpapp@kde.org>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "serialportreader.h"

#include <QCoreApplication>
#include <QString>
#include <QtCore>

QT_USE_NAMESPACE

//идекс позиции в массиве m_readData
int i = 0;

//Одна строка из пакета
QByteArray line;
//В ожидании доллора
bool flg = false;

//Широта
double latitude = 0;
//Полушарие з/в
QString latitude_sphere;

//Долгота
double longitude = 0;
//Полушарие с/в
QString longitude_sphere;

//Местное время
QTime local_time;
//Время по Гринвичу
QTime world_time;

//Горизонтальная скорость в км/ч
double speed;

//число спутников
int satellites_count;




SerialPortReader::SerialPortReader(QSerialPort *serialPort, QObject *parent)
    : QObject(parent)
    , m_serialPort(serialPort)
    , m_standardOutput(stdout)
{
    connect(m_serialPort, SIGNAL(readyRead()), SLOT(handleReadyRead()));
    connect(m_serialPort, SIGNAL(error(QSerialPort::SerialPortError)), SLOT(handleError(QSerialPort::SerialPortError)));
    connect(&m_timer, SIGNAL(timeout()), SLOT(handleTimeout()));

    m_timer.start(1000);
}

SerialPortReader::~SerialPortReader()
{
}

void SerialPortReader::handleReadyRead()
{
    m_readData.append(m_serialPort->readAll());

    if (!m_timer.isActive())
        m_timer.start(1);
}

void SerialPortReader::handleTimeout()
{
    if (m_readData.isEmpty()) {
        m_standardOutput << QObject::tr("No data was currently available for reading from port %1").arg(m_serialPort->portName()) << endl;
    }
    else {
        while (i < m_readData.length() ){
            //нашли начало строки
            if (m_readData[i] == '$'){
                flg = true;
            }
            //нашли конец строки
            else if (m_readData[i] == '\n'){
                //проверяем сообщение
                checkup(line);
                flg = false;
                line.clear();
            }
            //заполняем строку от $ до \n
            else if(flg){
                line = line.append(m_readData[i]);
            }
            i++;
        }


        m_standardOutput << "world_time: " << world_time.toString() << endl;
        m_standardOutput << "latitude: " << latitude << " " << latitude_sphere << endl;
        m_standardOutput << "longitude: " << longitude << " " << longitude_sphere << endl;
        m_standardOutput << "speed: " << speed << endl;
        m_standardOutput << "satellites_count: " << satellites_count << endl << endl;


        //очищаем порт
        m_readData.clear();
        //очищаем переменную для цикла while
        i = 0;
    }
}

void SerialPortReader::handleError(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError) {
        m_standardOutput << QObject::tr("An I/O error occurred while reading the data from port %1, error: %2").arg(m_serialPort->portName()).arg(m_serialPort->errorString()) << endl;
        QCoreApplication::exit(1);
    }
}


qint8 xhor;
QString xhor_row;

//проверка строки на достоверность
int checkup(QByteArray line){

    //Узнаем xhor
    xhor = line[0];
    for (int g = 1; g < line.length()-4; g++){
        xhor ^= line[g];
    }

    //Узнаем контрольную сумму
    if (line.length() > 5){
        xhor_row = line[line.length()-3];
        xhor_row=  xhor_row + line[line.length()-2];
    }

    //для проверки
    /*
    qInfo() << "XOR  = " << xhor << endl;
    qInfo() << "XOR row = " << xhor_row.toInt(0, 16) << endl;
    qInfo() << "line = " << line << endl << endl;*/

    //если сообщение пришло без помех
    if ( xhor == xhor_row.toInt(0, 16) ){
        parser(line);
    }
    else{
        qInfo() << "Не верное сообщение";
    }
    return 0;
}

//извличение информации из строки
int parser(QString line){
    QList<QString> line_part = line.split(',');
    QString msg_type = line_part.value(0);

    //Универсальная дичь
    QList<QString> buf;

    //местное время, широта, долгота, полушария, число спутников
    if (msg_type == "GPGGA"){

        buf = line_part.value(1).split('.');
        world_time =  QTime::fromString(buf.value(0), "hhmmss");


        buf = line_part.value(2).split('.');
        latitude = buf.value(0).toDouble()*0.01;

        latitude_sphere = line_part.value(3);

        buf = line_part.value(4).split('.');
        longitude = buf.value(0).toDouble()*0.01;

        longitude_sphere = line_part.value(5);

        satellites_count = line_part.value(7).toInt();
    }

    //узнаем скорость движения объекта
    if (msg_type == "GPVTG"){
        speed = line_part.value(7).toDouble();
    }
    return 0;
}
