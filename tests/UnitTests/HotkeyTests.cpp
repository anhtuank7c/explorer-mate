#include <CppUnitTest.h>

#include <string>

#include "Application/HotkeyMatcher.h"
#include "Application/Settings.h"
#include "Domain/KeyChord.h"

using Microsoft::VisualStudio::CppUnitTestFramework::Assert;

namespace et::tests {

namespace {

constexpr unsigned kLeftShift = 0xA0;
constexpr unsigned kLeftControl = 0xA2;
constexpr unsigned kRightControl = 0xA3;
constexpr unsigned kLeftAlt = 0xA4;
constexpr unsigned kRightAlt = 0xA5;

const auto kAlways = [](const domain::KeyChord&) { return true; };
const auto kNever = [](const domain::KeyChord&) { return false; };

app::KeyEvent Down(unsigned key) { return {key, true, false}; }
app::KeyEvent Up(unsigned key) { return {key, false, false}; }

app::HotkeyMatcher DefaultMatcher() {
    return app::HotkeyMatcher(app::Settings::Defaults().hotkeys);
}

void HoldCtrlAlt(app::HotkeyMatcher& matcher) {
    matcher.OnKey(Down(kLeftControl), kAlways);
    matcher.OnKey(Down(kLeftAlt), kAlways);
}

}  // namespace

TEST_CLASS(KeyChordTests) {
public:
    TEST_METHOD(FormatsAndParsesRoundTrip) {
        for (const wchar_t* text : {L"Ctrl+Alt+N", L"Ctrl+Shift+F5", L"Alt+7", L"Ctrl+Alt+Shift+Win+Z",
                                    L"Win+F24"}) {
            const auto chord = domain::ParseKeyChord(text);
            Assert::IsTrue(chord.ok(), text);
            Assert::AreEqual(std::wstring(text), domain::FormatKeyChord(chord.value()));
        }
    }

    TEST_METHOD(ParsingIgnoresCaseAndModifierOrder) {
        const auto chord = domain::ParseKeyChord(L"alt+CTRL+n");
        Assert::IsTrue(chord.ok());
        Assert::AreEqual(std::wstring(L"Ctrl+Alt+N"), domain::FormatKeyChord(chord.value()));
    }

    TEST_METHOD(RejectsUnsafeOrMalformedChords) {
        for (const wchar_t* text : {L"", L"N", L"Shift+N", L"Ctrl+", L"Ctrl+Alt", L"Ctrl+Enter",
                                    L"Ctrl+F0", L"Ctrl+F25", L"Ctrl+F01", L"Ctrl+NN", L"Ctrl++N",
                                    L"N+Ctrl", L"Ctrl+Alt+Ñ"}) {
            Assert::IsFalse(domain::ParseKeyChord(text).ok(), text);
        }
    }
};

TEST_CLASS(SettingsTests) {
public:
    TEST_METHOD(DefaultsAvoidCtrlD) {
        const app::Settings settings = app::Settings::Defaults();
        Assert::IsTrue(settings.hotkeysEnabled);
        Assert::AreEqual(std::wstring(L"Ctrl+Alt+N"),
                         domain::FormatKeyChord(*settings.ChordFor(app::ActionKind::GroupIntoNewFolder)));
        Assert::AreEqual(std::wstring(L"Ctrl+Alt+R"),
                         domain::FormatKeyChord(*settings.ChordFor(app::ActionKind::BulkRename)));
        Assert::AreEqual(std::wstring(L"Ctrl+Alt+D"),
                         domain::FormatKeyChord(*settings.ChordFor(app::ActionKind::DuplicateInPlace)));
        Assert::IsFalse(app::ValidateSettings(settings).has_value());
    }

    TEST_METHOD(RoundTripsIncludingDisabledAndUnboundActions) {
        app::Settings settings;
        settings.hotkeysEnabled = false;
        settings.hotkeys = {{app::ActionKind::BulkRename, domain::ParseKeyChord(L"Ctrl+Shift+F2").value()}};

        const auto parsed = app::ParseSettings(app::SerializeSettings(settings));

        Assert::IsTrue(parsed.ok());
        Assert::IsFalse(parsed.value().hotkeysEnabled);
        Assert::AreEqual(size_t{1}, parsed.value().hotkeys.size());
        Assert::IsFalse(parsed.value().ChordFor(app::ActionKind::DuplicateInPlace).has_value());
        Assert::AreEqual(std::wstring(L"Ctrl+Shift+F2"),
                         domain::FormatKeyChord(*parsed.value().ChordFor(app::ActionKind::BulkRename)));
    }

    TEST_METHOD(RejectsSameChordOnTwoActions) {
        app::Settings settings = app::Settings::Defaults();
        settings.hotkeys[1].chord = settings.hotkeys[0].chord;
        Assert::IsTrue(app::ValidateSettings(settings).has_value());
    }

    TEST_METHOD(RejectsMalformedSettingsText) {
        for (const wchar_t* text : {
                 L"",
                 L"ExplorerMate-Settings 2\nhotkeys=on\n",
                 L"ExplorerMate-Settings 1\nhotkeys=maybe\n",
                 L"ExplorerMate-Settings 1\nformat=Ctrl+Alt+F\n",
                 L"ExplorerMate-Settings 1\ngroup=N\n",
                 L"ExplorerMate-Settings 1\ngroup=Ctrl+Alt+N\nrename=Ctrl+Alt+N\n",
                 L"ExplorerMate-Settings 1\ngroup=Ctrl+Alt+N\ngroup=Ctrl+Alt+M\n",
                 L"ExplorerMate-Settings 1\nnonsense\n",
             }) {
            Assert::IsFalse(app::ParseSettings(text).ok(), text);
        }
    }
};

TEST_CLASS(HotkeyMatcherTests) {
public:
    TEST_METHOD(TriggersOnExactChordAndSwallowsKeyDownAndUp) {
        app::HotkeyMatcher matcher = DefaultMatcher();
        HoldCtrlAlt(matcher);

        const auto down = matcher.OnKey(Down('N'), kAlways);
        Assert::IsTrue(down.swallow);
        Assert::IsTrue(down.triggered == app::ActionKind::GroupIntoNewFolder);

        const auto up = matcher.OnKey(Up('N'), kAlways);
        Assert::IsTrue(up.swallow);
        Assert::IsFalse(up.triggered.has_value());
    }

    TEST_METHOD(ModifiersAreNeverSwallowed) {
        app::HotkeyMatcher matcher = DefaultMatcher();
        Assert::IsFalse(matcher.OnKey(Down(kLeftControl), kAlways).swallow);
        Assert::IsFalse(matcher.OnKey(Down(kLeftAlt), kAlways).swallow);
        matcher.OnKey(Down('D'), kAlways);
        Assert::IsFalse(matcher.OnKey(Up(kLeftAlt), kAlways).swallow);
        Assert::IsFalse(matcher.OnKey(Up(kLeftControl), kAlways).swallow);
    }

    TEST_METHOD(AutoRepeatDoesNotTriggerAgain) {
        app::HotkeyMatcher matcher = DefaultMatcher();
        HoldCtrlAlt(matcher);
        Assert::IsTrue(matcher.OnKey(Down('D'), kAlways).triggered.has_value());
        for (int repeat = 0; repeat < 5; ++repeat) {
            const auto again = matcher.OnKey(Down('D'), kAlways);
            Assert::IsTrue(again.swallow);
            Assert::IsFalse(again.triggered.has_value());
        }
        matcher.OnKey(Up('D'), kAlways);
        Assert::IsTrue(matcher.OnKey(Down('D'), kAlways).triggered.has_value());
    }

    TEST_METHOD(RequiresExactModifiers) {
        app::HotkeyMatcher matcher = DefaultMatcher();
        matcher.OnKey(Down(kLeftControl), kAlways);
        Assert::IsFalse(matcher.OnKey(Down('D'), kAlways).swallow);  // Ctrl+D is Explorer's Delete.
        matcher.OnKey(Up('D'), kAlways);

        matcher.OnKey(Down(kLeftAlt), kAlways);
        matcher.OnKey(Down(kLeftShift), kAlways);
        Assert::IsFalse(matcher.OnKey(Down('D'), kAlways).swallow);  // Extra Shift.
    }

    TEST_METHOD(OutsideTheFileListTheKeyPassesThroughUntouched) {
        app::HotkeyMatcher matcher = DefaultMatcher();
        HoldCtrlAlt(matcher);
        const auto down = matcher.OnKey(Down('R'), kNever);
        Assert::IsFalse(down.swallow);
        Assert::IsFalse(down.triggered.has_value());
        Assert::IsFalse(matcher.OnKey(Up('R'), kNever).swallow);
    }

    TEST_METHOD(AltGrNeverCountsAsAlt) {
        app::HotkeyMatcher matcher = DefaultMatcher();
        // Windows reports AltGr as Left Ctrl + Right Alt.
        matcher.OnKey(Down(kLeftControl), kAlways);
        matcher.OnKey(Down(kRightAlt), kAlways);
        Assert::IsFalse(matcher.OnKey(Down('N'), kAlways).swallow);
    }

    TEST_METHOD(RightControlWorksLikeLeft) {
        app::HotkeyMatcher matcher = DefaultMatcher();
        matcher.OnKey(Down(kRightControl), kAlways);
        matcher.OnKey(Down(kLeftAlt), kAlways);
        Assert::IsTrue(matcher.OnKey(Down('N'), kAlways).triggered.has_value());
    }

    TEST_METHOD(InjectedEventsAreIgnored) {
        app::HotkeyMatcher matcher = DefaultMatcher();
        matcher.OnKey({kLeftControl, true, true}, kAlways);
        matcher.OnKey({kLeftAlt, true, true}, kAlways);
        Assert::IsFalse(matcher.OnKey(Down('N'), kAlways).swallow);

        HoldCtrlAlt(matcher);
        Assert::IsFalse(matcher.OnKey({'N', true, true}, kAlways).swallow);
    }

    TEST_METHOD(ReleasingAModifierStopsMatching) {
        app::HotkeyMatcher matcher = DefaultMatcher();
        HoldCtrlAlt(matcher);
        matcher.OnKey(Up(kLeftAlt), kAlways);
        Assert::IsFalse(matcher.OnKey(Down('N'), kAlways).swallow);
    }

    TEST_METHOD(ResetForgetsHeldKeys) {
        app::HotkeyMatcher matcher = DefaultMatcher();
        HoldCtrlAlt(matcher);
        matcher.Reset();
        Assert::IsFalse(matcher.OnKey(Down('N'), kAlways).swallow);
    }

    // The caller compares the matched chord with the real keyboard state; a mismatch (a
    // modifier whose key-up was missed behind a UAC prompt) must let the key through.
    TEST_METHOD(ContextCheckReceivesTheMatchedChordAndCanVeto) {
        app::HotkeyMatcher matcher = DefaultMatcher();
        HoldCtrlAlt(matcher);
        domain::KeyChord seen;
        const auto veto = [&](const domain::KeyChord& chord) {
            seen = chord;
            return false;
        };
        const auto decision = matcher.OnKey(Down('D'), veto);
        Assert::IsFalse(decision.swallow);
        Assert::IsFalse(decision.triggered.has_value());
        Assert::AreEqual(std::wstring(L"Ctrl+Alt+D"), domain::FormatKeyChord(seen));
    }

    TEST_METHOD(UnboundActionHasNoShortcut) {
        app::HotkeyMatcher matcher({});
        HoldCtrlAlt(matcher);
        Assert::IsFalse(matcher.OnKey(Down('N'), kAlways).swallow);
    }
};

}  // namespace et::tests
