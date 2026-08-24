#include "OptionsGroup.h"
#include "../../Minecraft.h"
#include "ImageButton.h"
#include "OptionsItem.h"
#include "Slider.h"
#include "../../../locale/I18n.h"
#include "TextOption.h"
#include "KeyOption.h"
#include "../Screen.h"
#include "../../../platform/input/Mouse.h"
#include <cmath>

OptionsGroup::OptionsGroup( std::string labelID )  {
	label = I18n::get(labelID);
}

void OptionsGroup::setupPositions() {
	int curY = y + 18 - (int)scrollOffset;
	int curX = x + 10;
	
	// Determine if we should use 1 or 2 columns based on available width
	int cols = (width > 200) ? 2 : 1;
	int colWidth = (width - 15) / cols;
	
	int rowHeight = 0;
	
	for(size_t i = 0; i < children.size(); ++i) {
		GuiElement* child = children[i];
		child->width = colWidth - 5;
		
		int colIndex = i % cols;
		child->x = curX + (colIndex * colWidth);
		child->y = curY;
		child->setupPositions();
		
		if (child->height > rowHeight) {
			rowHeight = child->height;
		}
		
		if (colIndex == cols - 1 || i == children.size() - 1) {
			curY += rowHeight + 4; // Add vertical spacing
			rowHeight = 0;
		}
	}
	contentHeight = (curY + (int)scrollOffset) - y;
}

void OptionsGroup::render( Minecraft* minecraft, int xm, int ym ) {
	float padX = 10.0f;
	float padY = 5.0f;

	if (Mouse::isButtonDown(MouseAction::ACTION_LEFT)) {
		if (!isDragging) {
			if (xm >= x && xm <= x + width && ym >= y && ym <= y + height) {
				isDragging = true;
				lastMouseY = ym;
				scrollVelocity = 0.0f;
				totalDragAmount = 0.0f;
			}
		} else {
			int dy = lastMouseY - ym;
			if (dy != 0) {
				scrollOffset += dy;
				scrollVelocity = (float)dy;
				totalDragAmount += dy;
				setupPositions();
			}
			lastMouseY = ym;
		}
	} else {
		isDragging = false;
		if (scrollVelocity != 0.0f) {
			scrollOffset += scrollVelocity;
			scrollVelocity *= 0.8f;
			if (std::abs(scrollVelocity) < 0.5f) scrollVelocity = 0.0f;
			setupPositions();
		}
	}

	int screenH = minecraft->screen->height;
	int visibleHeight = screenH - y - 18;
	int maxScroll = contentHeight - visibleHeight;
	if (maxScroll < 0) maxScroll = 0;

	if (scrollOffset < 0.0f) {
		scrollOffset *= 0.8f;
		if (scrollOffset > -1.0f) scrollOffset = 0.0f;
		setupPositions();
	} else if (scrollOffset > maxScroll) {
		float diff = scrollOffset - maxScroll;
		diff *= 0.8f;
		scrollOffset = maxScroll + diff;
		if (diff < 1.0f) scrollOffset = (float)maxScroll;
		setupPositions();
	}
	
    float labelY = (float)y + padY - scrollOffset;
    int visibleBottom = minecraft->screen->height;
    if (labelY > y - 20 && labelY < visibleBottom) {
	    minecraft->font->draw(label, (float)x + padX, labelY, 0xffffffff, false);
    }

	for(std::vector<GuiElement*>::iterator it = children.begin(); it != children.end(); ++it) {
        if ((*it)->y + (*it)->height < this->y || (*it)->y > visibleBottom) continue;
		(*it)->render(minecraft, xm, ym);
	}
}

OptionsGroup& OptionsGroup::addOptionItem(OptionId optId, Minecraft* minecraft ) {
	auto option = minecraft->options.getOpt(optId);

	if (option == nullptr) return *this;

	// TODO: do a options key class to check it faster via dynamic_cast
	if (option->getStringId().find("options.key") != std::string::npos) createKey(optId, minecraft);
	else if (dynamic_cast<OptionBool*>(option)) createToggle(optId, minecraft);
	else if (dynamic_cast<OptionFloat*>(option)) createProgressSlider(optId, minecraft);
	else if (dynamic_cast<OptionInt*>(option)) createStepSlider(optId, minecraft);
	else if (dynamic_cast<OptionString*>(option)) createTextbox(optId, minecraft);

	return *this;
}

void OptionsGroup::tick(Minecraft* minecraft) {
	super::tick(minecraft);
}

void OptionsGroup::mouseReleased(Minecraft* minecraft, int x, int y, int buttonNum) {
    if (std::abs(totalDragAmount) > 10.0f) {
        totalDragAmount = 0.0f;
        return;
    }
    totalDragAmount = 0.0f;
    super::mouseReleased(minecraft, x, y, buttonNum);
}

// TODO: wrap this copypaste shit into templates

void OptionsGroup::createToggle(OptionId optId, Minecraft* minecraft ) {
	ImageDef def;

	def.setSrc(IntRectangle(160, 206, 39, 20));
	def.name = "gui/touchgui.png";
	def.width = 39 * 0.7f;
	def.height = 20 * 0.7f;
	
	OptionButton* element = new OptionButton(optId);
	element->setImageDef(def, true);
	element->updateImage(&minecraft->options);
	
	std::string itemLabel = I18n::get(minecraft->options.getOpt(optId)->getStringId());
	
	OptionsItem* item = new OptionsItem(optId, itemLabel, element);
	
	addChild(item);
	setupPositions();
}

void OptionsGroup::createProgressSlider(OptionId optId, Minecraft* minecraft ) {
	Slider* element = new SliderFloat(minecraft, optId);
	element->width = 100;
	element->height = 20;

	std::string itemLabel = I18n::get(minecraft->options.getOpt(optId)->getStringId());
	OptionsItem* item = new OptionsItem(optId, itemLabel, element);
	addChild(item);
	setupPositions();
}

void OptionsGroup::createStepSlider(OptionId optId, Minecraft* minecraft ) {
	Slider* element = new SliderInt(minecraft, optId);
	element->width = 100;
	element->height = 20;
	std::string itemLabel = I18n::get(minecraft->options.getOpt(optId)->getStringId());
	OptionsItem* item = new OptionsItem(optId, itemLabel, element);
	addChild(item);
	setupPositions();
}

void OptionsGroup::createTextbox(OptionId optId, Minecraft* minecraft) {
	TextBox* element = new TextOption(minecraft, optId);
	element->width = 100;
	element->height = 20;

	std::string itemLabel = I18n::get(minecraft->options.getOpt(optId)->getStringId());
	OptionsItem* item = new OptionsItem(optId, itemLabel, element);
	addChild(item);
	setupPositions();
}

void OptionsGroup::createKey(OptionId optId, Minecraft* minecraft) {
	KeyOption* element = new KeyOption(minecraft, optId);
	element->width = 50;
	element->height = 20;

	std::string itemLabel = I18n::get(minecraft->options.getOpt(optId)->getStringId());
	OptionsItem* item = new OptionsItem(optId, itemLabel, element);
	addChild(item);
	setupPositions();
}
