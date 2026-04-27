#pragma once

#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QtGlobal>

// OHQ / Excel epoch: day-serial 0 = 1899-12-30 UTC.
// Used by the CSV loader and by code that reads OHQ-style day-serial
// timestamps from OHQ state JSON files.
inline qint64 ohqSerialToMsEpoch(double serial)
{
    static const QDateTime kOHQEpoch(
        QDate(1899, 12, 30),
        QTime(0, 0, 0),
        Qt::UTC);

    const qint64 offsetMs = qRound64(serial * 86400.0 * 1000.0);
    return kOHQEpoch.addMSecs(offsetMs).toMSecsSinceEpoch();
}
