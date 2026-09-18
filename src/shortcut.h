#pragma once

#include <QAction>
#include <QKeySequence>
#include <QObject>

class Shortcut : public QObject
{
    Q_OBJECT

public:
    explicit Shortcut(QObject *parent = nullptr);

    void registerShortcut(const QKeySequence &sequence = QKeySequence());

    bool setShortcut(const QKeySequence &sequence);

    QKeySequence currentSequence() const { return m_sequence; }

signals:
    void activated();
    void conflictDetected(const QKeySequence &sequence);

private:
    bool registerSequence(const QKeySequence &sequence);
    void unregister();

    QAction *m_action = nullptr;
    QKeySequence m_sequence;
};