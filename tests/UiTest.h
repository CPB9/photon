#pragma once

#include "decode/core/Rc.h"
#include "decode/groundcontrol/Exchange.h"

#include <QObject>

#include <cstdint>
#include <cstddef>

class DataStream : public QObject, public decode::DataSink {
    Q_OBJECT
public:
    virtual int64_t readData(void* dest, std::size_t size) = 0;
    virtual int64_t bytesAvailable() const = 0;

signals:
    void readyRead();
};

class QApplication;

int runUiTest(QApplication* app, DataStream* stream);
