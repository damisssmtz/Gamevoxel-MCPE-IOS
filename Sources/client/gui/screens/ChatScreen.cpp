#include "ChatScreen.h"
#include "../Gui.h"
#include "../../Minecraft.h"
#include "../components/TextBox.h"
#include "../components/Button.h"
#include "../../../AppPlatform.h"
#include "../../../platform/input/Keyboard.h"

ChatScreen::ChatScreen() : chatBox(NULL), sendBtn(NULL) {}

ChatScreen::~ChatScreen() {
    delete chatBox;
    delete sendBtn;
}

void ChatScreen::init() {
    minecraft->platform()->showKeyboard();

    chatBox = new TextBox(0, 4, 20, width - 40 - 8, 26, "");
    chatBox->focused = true;
    chatBox->hint = ">";

    sendBtn = new Button(1, width - 40, 20, 36, 26, ">");

    textBoxes.push_back(chatBox);
    buttons.push_back(sendBtn);

    setupPositions();
}

void ChatScreen::setupPositions() {
    if (chatBox) {
        chatBox->x = 4;
        chatBox->y = 20;
        chatBox->width = width - 40 - 8;
    }
    if (sendBtn) {
        sendBtn->x = width - 40;
        sendBtn->y = 20;
    }
}

void ChatScreen::render(int xm, int ym, float a) {
    fill(0, 14, width, 50, 0x80000000);
    Screen::render(xm, ym, a);
}

void ChatScreen::buttonClicked(Button* button) {
    if (button->id == 1) { // Send
        if (chatBox->text.length() > 0) {
            minecraft->gui.addMessage(chatBox->text);
        }
        minecraft->platform()->hideKeyboard();
        minecraft->setScreen(NULL);
    }
}

void ChatScreen::keyPressed(int eventKey) {
    if (eventKey == Keyboard::KEY_ESCAPE) {
        minecraft->platform()->hideKeyboard();
        minecraft->setScreen(NULL);
    } else if (eventKey == Keyboard::KEY_RETURN) {
        if (chatBox->text.length() > 0) {
            minecraft->gui.addMessage(chatBox->text);
        }
        minecraft->platform()->hideKeyboard();
        minecraft->setScreen(NULL);
    } else {
        Screen::keyPressed(eventKey);
    }
}
