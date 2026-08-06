#ifndef CONSOLEINPUTPARSER_H
#define CONSOLEINPUTPARSER_H

enum class PlaybackKey {
    None, Left, Right, Home, End, PageUp, PageDown, Quit, Pause
};

struct PlaybackKeyEvent {
    PlaybackKey key;
    PlaybackKeyEvent(PlaybackKey k = PlaybackKey::None) : key(k) {}
};

class ConsoleInputParser {
public:
    PlaybackKeyEvent feed(unsigned char c);

    PlaybackKeyEvent timeout();

    bool pending() const { return m_state != State::Idle; }

private:
    enum class State { Idle, Esc, EscBracket, EscBracketDigit, EscO };

    State m_state = State::Idle;
    int m_digit = 0;

    void reset() { m_state = State::Idle; m_digit = 0; }
};

#endif
