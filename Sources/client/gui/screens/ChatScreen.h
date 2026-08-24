#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__ChatScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__ChatScreen_H__

#include "../Screen.h"

class TextBox;
class Button;

class ChatScreen: public Screen
{
public:
	ChatScreen();
	virtual ~ChatScreen();

	void init();
	void setupPositions();
	void render(int xm, int ym, float a);

	void buttonClicked(Button* button);
	void keyPressed(int eventKey);

    virtual bool renderGameBehind() { return true; }

private:
	TextBox* chatBox;
	Button* sendBtn;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__ChatScreen_H__*/
