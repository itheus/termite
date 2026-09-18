#pragma once

#include <QObject>
#include <QString>

class QProcess;

class Capture : public QObject
{
    Q_OBJECT

public:
    explicit Capture(QObject *parent = nullptr);

    void capture();

    QString prepare(const QString &path) const;

    void discard(const QString &path);

signals:
    void finished(const QString &path);

private:
    QString outputPath() const;
    void ensureRuntimeDir() const;

    QProcess *m_process = nullptr;
    QString m_requestedPath;
};