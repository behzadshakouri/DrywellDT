/*
 * OpenHydroQual - Environmental Modeling Platform
 * Copyright (C) 2025 Arash Massoudieh
 * 
 * This file is part of OpenHydroQual.
 * 
 * OpenHydroQual is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 * 
 * If you use this file in a commercial product, you must purchase a
 * commercial license. Contact arash.massoudieh@cua.edu for details.
 */

#include "System.h"
#include "Script.h"
#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <algorithm>
#include <iostream>
#include <string>

using namespace std;

namespace {

struct AppOptions
{
    string scriptFile;
    string weatherFile;
    string stateFile;
    int runDays = 1;
    bool watchDaily = false;
};

bool SaveJsonToFile(const QJsonObject &obj, const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        cerr << "Could not write state file: " << path.toStdString() << endl;
        return false;
    }
    file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    return true;
}

QJsonObject LoadJsonFromFile(const QString &path)
{
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
    {
        return QJsonObject();
    }

    const auto doc = QJsonDocument::fromJson(file.readAll());
    return doc.isObject() ? doc.object() : QJsonObject();
}

QJsonArray LoadWeatherArray(const QString &weatherPath)
{
    if (weatherPath.isEmpty())
        return QJsonArray();

    const auto weatherObj = LoadJsonFromFile(weatherPath);
    if (weatherObj.contains("days") && weatherObj["days"].isArray())
    {
        return weatherObj["days"].toArray();
    }

    return QJsonArray();
}

QJsonObject SelectWeatherForDate(const QJsonArray &weatherDays, const QString &isoDate)
{
    for (const auto &entry : weatherDays)
    {
        if (!entry.isObject())
            continue;
        const auto obj = entry.toObject();
        if (obj.value("date").toString() == isoDate)
        {
            return obj;
        }
    }

    return QJsonObject();
}

void PrintUsage()
{
    cout << "Usage:\n"
         << "  OpenHydroQual-Console <script_file> [options]\n\n"
         << "Options:\n"
         << "  --weather <file.json>   Weather inputs with a 'days' array\n"
         << "  --state <file.json>     Save/load model run state\n"
         << "  --days <n>              Number of day-solves to run immediately\n"
         << "  --watch-daily           Keep process running and solve every 24h\n"
         << endl;
}

bool ParseArgs(int argc, char *argv[], AppOptions &options)
{
    if (argc < 2)
    {
        PrintUsage();
        return false;
    }

    options.scriptFile = argv[1];
    for (int i = 2; i < argc; ++i)
    {
        const string arg = argv[i];

        if (arg == "--weather" && i + 1 < argc)
        {
            options.weatherFile = argv[++i];
        }
        else if (arg == "--state" && i + 1 < argc)
        {
            options.stateFile = argv[++i];
        }
        else if (arg == "--days" && i + 1 < argc)
        {
            options.runDays = max(1, stoi(argv[++i]));
        }
        else if (arg == "--watch-daily")
        {
            options.watchDaily = true;
        }
        else
        {
            cerr << "Unknown or incomplete argument: " << arg << "\n";
            PrintUsage();
            return false;
        }
    }

    if (options.scriptFile.empty())
    {
        PrintUsage();
        return false;
    }

    return true;
}

bool BuildAndSolveSystem(const AppOptions &options, const QString &solveDateIso, const QJsonObject &weatherForDay)
{
    cout << "Input file: " << options.scriptFile << endl;
    System *system = new System();
    cout << "Reading script ..." << endl;

    string defaulttemppath = qApp->applicationDirPath().toStdString() + "/../../resources/";
    cout << "Default Template path = " + defaulttemppath + "\n";

    system->SetDefaultTemplatePath(defaulttemppath);
    system->SetWorkingFolder(QFileInfo(QString::fromStdString(options.scriptFile)).canonicalPath().toStdString() + "/");

    string settingfilename = qApp->applicationDirPath().toStdString() + "/../../resources/settings.json";
    Script scr(options.scriptFile, system);

    cout << "Executing script ..." << endl;
    system->CreateFromScript(scr, settingfilename);
    system->SetSilent(false);

    cout << "Solving daily period for " << solveDateIso.toStdString() << " ..." << endl;
    if (!weatherForDay.isEmpty())
    {
        cout << "Weather data loaded for " << solveDateIso.toStdString() << " (keys: ";
        const auto keys = weatherForDay.keys();
        for (int i = 0; i < keys.size(); ++i)
        {
            cout << keys[i].toStdString();
            if (i + 1 < keys.size())
                cout << ", ";
        }
        cout << ")\n";
    }

    system->Solve();
    cout << "Writing outputs in '" << system->GetWorkingFolder() + system->OutputFileName() << "'";
    system->GetOutputs().write(system->GetWorkingFolder() + system->OutputFileName());
    delete system;
    return true;
}

QJsonObject BuildNextState(const AppOptions &options, const QString &lastRunDate, int totalRuns)
{
    QJsonObject state;
    state["script"] = QString::fromStdString(options.scriptFile);
    state["weather"] = QString::fromStdString(options.weatherFile);
    state["last_run_date"] = lastRunDate;
    state["next_run_date"] = QDate::fromString(lastRunDate, Qt::ISODate).addDays(1).toString(Qt::ISODate);
    state["runs_completed"] = totalRuns;
    state["updated_at_utc"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    return state;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    AppOptions options;
    if (!ParseArgs(argc, argv, options))
    {
        return 1;
    }

    const QString weatherPath = QString::fromStdString(options.weatherFile);
    const QJsonArray weatherDays = LoadWeatherArray(weatherPath);

    const QString statePath = QString::fromStdString(options.stateFile);
    QJsonObject state = LoadJsonFromFile(statePath);

    QDate solveDate = QDate::currentDate();
    if (state.contains("next_run_date"))
    {
        const QDate loadedDate = QDate::fromString(state.value("next_run_date").toString(), Qt::ISODate);
        if (loadedDate.isValid())
        {
            solveDate = loadedDate;
        }
    }

    int totalRuns = state.value("runs_completed").toInt(0);

    auto runForDay = [&]() {
        const QString dayIso = solveDate.toString(Qt::ISODate);
        const QJsonObject weatherForDay = SelectWeatherForDate(weatherDays, dayIso);
        BuildAndSolveSystem(options, dayIso, weatherForDay);

        ++totalRuns;
        const QJsonObject updatedState = BuildNextState(options, dayIso, totalRuns);
        if (!statePath.isEmpty())
        {
            SaveJsonToFile(updatedState, statePath);
        }

        solveDate = solveDate.addDays(1);
    };

    for (int i = 0; i < options.runDays; ++i)
    {
        runForDay();
    }

    if (options.watchDaily)
    {
        QTimer timer;
        QObject::connect(&timer, &QTimer::timeout, &a, runForDay);
        timer.start(24 * 60 * 60 * 1000);
        cout << "watch-daily enabled. Next run date: " << solveDate.toString(Qt::ISODate).toStdString() << endl;
        return a.exec();
    }

    return 0;
}
