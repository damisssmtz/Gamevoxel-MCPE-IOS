#ifndef NET_MINECRAFT_CLIENT_GUI_COMPONENTS__OptionsGroup_H__
#define NET_MINECRAFT_CLIENT_GUI_COMPONENTS__OptionsGroup_H__

//package net.minecraft.client.gui;

#include <string>
#include "GuiElementContainer.h"
#include "ScrollingPane.h"
#include "../../Options.h"

class Font;
class Minecraft;

class OptionsGroup: public GuiElementContainer {
	typedef GuiElementContainer super;
public:
	OptionsGroup(std::string labelID);
	virtual void setupPositions();
	virtual void render(Minecraft* minecraft, int xm, int ym);
	virtual void tick(Minecraft* minecraft);
	OptionsGroup& addOptionItem(OptionId optId, Minecraft* minecraft);

    float scrollOffset = 0.0f;
    float scrollVelocity = 0.0f;
    bool isDragging = false;
    int lastMouseY = 0;
    int contentHeight = 0;
    float totalDragAmount = 0.0f;

    virtual void mouseReleased(Minecraft* minecraft, int x, int y, int buttonNum) override;
    virtual void mouseWheel(int dx, int dy, int xm, int ym) {}
protected:

	void createToggle(OptionId optId, Minecraft* minecraft);
	void createProgressSlider(OptionId optId, Minecraft* minecraft);
	void createStepSlider(OptionId optId, Minecraft* minecraft);
	void createTextbox(OptionId optId, Minecraft* minecraft);
	void createKey(OptionId optId, Minecraft* minecraft);

	std::string label;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_COMPONENTS__OptionsGroup_H__*/
