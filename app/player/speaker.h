#pragma once

#include <QObject>
#include <QString>

class QTextToSpeech;

// Japanese speech output (Qt TextToSpeech) for the glossary's pronunciation. Without the module
// (a build without Qt6::TextToSpeech, MITORI_SPEECH unset) or without a Japanese voice on the
// device it stays silent and says so in state().
class Speaker : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Unsupported,    // built without speech output, or no engine but the test one
        Starting,       // the engine is still loading its voices
        NoVoice,        // no Japanese voice (or the engine failed)
        Ready,
    };

    // `engine`: a Qt TextToSpeech engine by name ("mock" in tests), empty for the platform's.
    explicit Speaker(const QString &engine = {}, QObject *parent = nullptr);

    State state() const { return m_state; }
    // Speaks `text` with the Japanese voice, cutting off what is still being said.
    void say(const QString &text);

signals:
    void stateChanged();

private:
    void update();

    QTextToSpeech *m_speech = nullptr;
    State m_state = State::Unsupported;
};
