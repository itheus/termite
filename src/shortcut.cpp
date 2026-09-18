#include "shortcut.h"

#include <QAction>
#include <QDebug>

#include <KGlobalAccel>

namespace {
const QKeySequence kDefaultSequence(Qt::META | Qt::Key_M);

const char *kActionName = "termite-toggle";
const char *kActionText = "Toggle Termite";
} // namespace

Shortcut::Shortcut(QObject *parent)
    : QObject(parent)
{
}

void Shortcut::registerShortcut(const QKeySequence &sequence)
{
    m_sequence = sequence.isEmpty() ? kDefaultSequence : sequence;
    registerSequence(m_sequence);
}

bool Shortcut::setShortcut(const QKeySequence &sequence)
{
    if (sequence.isEmpty() || sequence == m_sequence) {
        return true;
    }

    unregister();

    const auto conflicts = KGlobalAccel::globalShortcutsByKey(sequence, KGlobalAccel::Equal);
    if (!conflicts.isEmpty()) {
        qInfo() << "Termite: requested shortcut" << sequence.toString()
                << "is owned by another component; reclaiming it.";
        KGlobalAccel::stealShortcutSystemwide(sequence);
    }

    const bool ok = registerSequence(sequence);
    if (ok) {
        m_sequence = sequence;
    } else {
        qWarning() << "Termite: failed to apply shortcut" << sequence.toString()
                   << "; restoring previous binding.";
        registerSequence(m_sequence);
        emit conflictDetected(sequence);
    }
    return ok;
}

bool Shortcut::registerSequence(const QKeySequence &sequence)
{
    if (!m_action) {
        m_action = new QAction(this);
        m_action->setObjectName(QString::fromLatin1(kActionName));
        m_action->setText(QString::fromLatin1(kActionText));
        connect(m_action, &QAction::triggered, this, &Shortcut::activated);
    }

    auto *accel = KGlobalAccel::self();

    const auto conflicts = KGlobalAccel::globalShortcutsByKey(sequence, KGlobalAccel::Equal);
    if (!conflicts.isEmpty()) {
        KGlobalAccel::stealShortcutSystemwide(sequence);
    }

    const QList<QKeySequence> sequences{sequence};
    accel->setDefaultShortcut(m_action, sequences, KGlobalAccel::NoAutoloading);
    const bool ok = accel->setShortcut(m_action, sequences, KGlobalAccel::NoAutoloading);

    if (ok) {
        qInfo() << "Termite: registered global shortcut" << sequence.toString();
    } else {
        qWarning() << "Termite: failed to register global shortcut" << sequence.toString();
    }
    return ok;
}

void Shortcut::unregister()
{
    if (!m_action) {
        return;
    }
    KGlobalAccel::self()->removeAllShortcuts(m_action);
}