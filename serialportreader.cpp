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

//индексы входа и выхода GPGLL
int g1 = 0;
int g2 = 0;

QByteArray line_GPGLL;
QByteArray line_GPRMC;


//идекс позиции в массиве m_readData
int i = 0;

//Широта
double latitude = 0;
//Полушарие з/в
QString latitude_sphere;

//Долгота
double longitude = 0;
//Полушарие с/в
QString longitude_sphere;

//Местное время
double local_time;

//Одна строка из пакета
QByteArray line;
//Вожидании доллора
bool flg = false;

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
            if (m_readData[i] == '$'){
                flg = true;
            }
            //заполняем строку от $ до \n
            else if (m_readData[i] == '\n'){
                parser(line);

                //m_standardOutput << m_readData << endl;
                /*
                m_standardOutput << "time = " << local_time << endl;
                m_standardOutput << "latitude = " << latitude << endl;
                m_standardOutput << "latitude_sphere = " << latitude_sphere << endl;
                m_standardOutput << "longitude = " << longitude << endl;
                m_standardOutput << "longitude_sphere = " << longitude_sphere << endl;
                */
                flg = false;
                line.clear();
            }

            else if(flg){
                line = line.append(m_readData[i]);
            }

            i++;
        }


        m_standardOutput << "time = " << local_time << endl;
        m_standardOutput << "latitude = " << latitude << " " << latitude_sphere << endl;
        m_standardOutput << "longitude = " << longitude << " " << longitude_sphere << endl << endl;


/*
        QByteArray y("$GPGLL");
        g1 = m_readData.indexOf(y);
        QByteArray z("$GPTXT");
        g2 = m_readData.indexOf(z);
        for (int i = g1; i < g2; ++i) {
            line_GPGLL = line_GPGLL.append(m_readData[i]);
        }
        m_standardOutput << line_GPGLL << endl;

        QByteArray t("$GPRMC");
        g1 = m_readData.indexOf(t);
        QByteArray v("$GPVTG");
        g2 = m_readData.indexOf(v);
        for (int i = g1; i < g2; ++i) {
            line_GPRMC = line_GPRMC.append(m_readData[i]);
        }
        m_standardOutput << line_GPRMC << endl;



        QString str_GPGLL=line_GPGLL;
        QList<QString> test = str_GPGLL.split(',');

        QList<QString> latitude = test.value(1).split('.');
        double lat=latitude.value(0).toDouble()*0.01;

        QList<QString> longitude = test.value(3).split('.');
        double lon=longitude.value(0).toDouble()*0.01;
        m_standardOutput << "Latitude: " << lat << " Longitude: " << lon << endl;
*/

        //очищаем порт
        m_readData.clear();
        i = 0;

        //line_GPGLL.clear();
        //line_GPRMC.clear();
        //m_standardOutput << QObject::tr("Data successfully received from port %1").arg(m_serialPort->portName()) << endl;
        //m_standardOutput << m_readData << endl;
    }
}

void SerialPortReader::handleError(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError) {
        m_standardOutput << QObject::tr("An I/O error occurred while reading the data from port %1, error: %2").arg(m_serialPort->portName()).arg(m_serialPort->errorString()) << endl;
        QCoreApplication::exit(1);
    }
}

int parser(QString line){
    QList<QString> test = line.split(',');
    QString msg_type = test.value(0);

    //Универсальная дичь
    QList<QString> buf;

    if (msg_type == "GPGGA"){
        buf = test.value(1).split('.');
        local_time = buf.value(0).toDouble();
        buf.clear();

        buf = test.value(2).split('.');
        latitude = buf.value(0).toDouble()*0.01;

        latitude_sphere = test.value(3);

        buf = test.value(4).split('.');
        longitude = buf.value(0).toDouble()*0.01;

        latitude_sphere = test.value(5);
    }

    return 0;
}
