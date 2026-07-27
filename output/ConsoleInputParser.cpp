#include "ConsoleInputParser.h"

PlaybackKeyEvent ConsoleInputParser::feed(unsigned char c)
{
    switch (m_state) {
    case State::Idle:
        if (c == 0x1b) { m_state = State::Esc; return {}; }
        if (c == 'q' || c == 'Q') return { PlaybackKey::Quit };
        return {};
    case State::Esc:
        if (c == '[') { m_state = State::EscBracket; return {}; }
        if (c == 'O') { m_state = State::EscO; return {}; }
        reset();
        return feed(c);
    case State::EscBracket:
        switch (c) {
        case 'C': reset(); return { PlaybackKey::Right };
        case 'D': reset(); return { PlaybackKey::Left };
        case 'H': reset(); return { PlaybackKey::Home };
        case 'F': reset(); return { PlaybackKey::End };
        }
        if (c >= '0' && c <= '9') {
            m_digit = c - '0';
            m_state = State::EscBracketDigit;
            return {};
        }
        reset();
        return {};
    case State::EscBracketDigit:
        if (c >= '0' && c <= '9') {
            m_digit = m_digit * 10 + (c - '0');
            return {};
        }
        if (c == '~') {
            PlaybackKey key = PlaybackKey::None;
            switch (m_digit) {
            case 1: case 7: key = PlaybackKey::Home; break;
            case 4: case 8: key = PlaybackKey::End; break;
            case 5: key = PlaybackKey::PageUp; break;
            case 6: key = PlaybackKey::PageDown; break;
            }
            reset();
            return { key };
        }
        reset();
        return {};
    case State::EscO:
        switch (c) {
        case 'H': reset(); return { PlaybackKey::Home };
        case 'F': reset(); return { PlaybackKey::End };
        }
        reset();
        return {};
    }
    return {};
}

PlaybackKeyEvent ConsoleInputParser::timeout()
{
    bool wasBareEsc = (m_state == State::Esc);
    reset();
    return wasBareEsc ? PlaybackKeyEvent{ PlaybackKey::Quit } : PlaybackKeyEvent{};
}
