#pragma once

#include <QDate>
#include <QDateTime>
#include <QTime>

// OHQ / Excel epoch: day-serial 0 = 1899-12-30 UTC.
// Used by both the CSV loader and any code that reads OHQ-style
// day-serial timestamps out of OHQ state JSON files.
inline qint64 ohqSerialToMsEpoch(double serial)
{
    static const QDateTime kOHQEpoch =
        QDateTime(QDate(1899, 12, 30), QTime(0, 0, 0), Qt::UTC);
    const qint64 offsetMs = static_cast<qint64>(serial * 86400.0 * 1000.0);
    return kOHQEpoch.addMSecs(offsetMs).toMSecsSinceEpoch();
}
