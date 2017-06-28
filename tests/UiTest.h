#pragma once

namespace decode {
class DataStream;
}

class QApplication;

int runUiTest(QApplication* app, decode::DataStream* stream);
