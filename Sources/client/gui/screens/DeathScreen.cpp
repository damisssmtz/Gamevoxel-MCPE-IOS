#include "DeathScreen.h"
#include "ScreenChooser.h"
#include "../components/Button.h"
#include "../../Minecraft.h"
#include "../../player/LocalPlayer.h"
#include "../../../platform/mc_time.h"
#include <sstream>

static const int WAIT_TICKS = 33; // ~0.55s a 60fps

DeathScreen::DeathScreen()
:	bRespawn(0),
	bTitle(0),
	_hasChosen(false),
	_tick(0)
{
}

DeathScreen::~DeathScreen()
{
	delete bRespawn;
	delete bTitle;
}

void DeathScreen::init()
{
	if (minecraft->options.getIntValue(OPTIONS_MENU_STYLE) == 0) {
		bRespawn = new Touch::TButton(1, "REAPARECER");
		bTitle = new Touch::TButton(2, "VOLVER AL MENU");
	} else if (minecraft->options.getIntValue(OPTIONS_MENU_STYLE) == 1) {
		bRespawn = new Button(1, "REAPARECER");
		bTitle = new Button(2, "VOLVER AL MENU");
	} else {
		bRespawn = new Button(1, 0, 0, 200, 20, "REAPARECER");
		bTitle = new Button(2, 0, 0, 200, 20, "VOLVER AL MENU");
	}
    
    // Bloquear botones al inicio 
    bRespawn->active = false;
    bTitle->active = false;
    bRespawn->visible = false;
    bTitle->visible = false;

	buttons.push_back(bRespawn);
	buttons.push_back(bTitle);

	tabButtons.push_back(bRespawn);
	tabButtons.push_back(bTitle);
}

void DeathScreen::setupPositions()
{
	// Agruparemos todos los elementos visuales anclándolos a la posición de los botones
	// para asegurar de que el espaciado interno sea siempre el mismo y se vea perfecto.
	int btnY = (height / 2) + 20;

	if (minecraft->options.getIntValue(OPTIONS_MENU_STYLE) == 2){
		bRespawn->width = 200;
		bTitle->width = 200;

		int centerX = (width / 2) - (bRespawn->width / 2);
		bRespawn->x = centerX;
		bTitle->x = centerX;

		bRespawn->y = btnY;
		bTitle->y = bRespawn->y + 26;
	} else {
		bRespawn->width = bTitle->width = width / 4;
		bRespawn->y = bTitle->y = btnY;
		bRespawn->x = width/2 - bRespawn->width - 10;
		bTitle->x = width/2 + 10;
	}
}

void DeathScreen::tick() {
	++_tick;
    // Desbloquear botones al pasar el delay
    if (_tick == WAIT_TICKS) {
        bRespawn->active = true;
        bTitle->active = true;
        bRespawn->visible = true;
        bTitle->visible = true;
    }
}

std::string formatScore(int value) {
    if (value < 0) value = 0;
    std::stringstream ss;
    ss << value;
    std::string s = ss.str();
    int insertPosition = s.length() - 3;
    while (insertPosition > 0) {
        s.insert(insertPosition, ".");
        insertPosition -= 3;
    }
    return s;
}

void DeathScreen::render( int xm, int ym, float a )
{
	// Fondo general con oscurecimiento y un tinte rojizo sutil
	fillGradient(0, 0, width, height, 0x90400000, 0xC0100000);

    // Animación de entrada suave
    float scale = 1.0f;
    if (_tick < WAIT_TICKS) {
        float t = (float)_tick / WAIT_TICKS;
        float inv = 1.0f - t;
        t = 1.0f - (inv * inv * inv * inv * inv); // Ease out quint
        scale = 0.92f + (0.08f * t);
    }

    glPushMatrix2();
    glTranslatef2(width/2, height/2, 0);
    glScalef2(scale, scale, 1.0f);
    glTranslatef2(-width/2, -height/2, 0);

    // Anclamos todo el grupo de textos a la posición de 'bRespawn'
    // Esto previene solapamientos si cambia la altura de la ventana
    int baseY = bRespawn->y; 

    // Panel translúcido detrás del texto (Simulando DeathPanel)
    int panelWidth = 260; 
    if (panelWidth > width - 20) panelWidth = width - 20;
    int panelTop = baseY - 100;
    int panelBottom = baseY + (minecraft->options.getIntValue(OPTIONS_MENU_STYLE) == 2 ? 60 : 30);
    fillGradient(width/2 - panelWidth/2, panelTop, width/2 + panelWidth/2, panelBottom, 0x40190507, 0x50FF4C4C);

    // Ícono de muerte superior "✕"
    glPushMatrix2();
	glScalef2(2.5f, 2.5f, 2.5f);
	drawCenteredString(font, "X", width / 2 / 2.5f, (baseY - 85) / 2.5f, 0xFFF7F7);
	glPopMatrix2();

	// Título principal
	glPushMatrix2();
	glScalef2(2.0f, 2.0f, 2.0f);
	drawCenteredString(font, "HAS MUERTO", width / 2 / 2.0f, (baseY - 55) / 2.0f, 0xFFF7F7);
	glPopMatrix2();

	// Mensaje secundario
	drawCenteredString(font, "Tu aventura aun no ha terminado.", width / 2, baseY - 32, 0xE8DFDF);

	// Puntuación
	std::stringstream ss;
	ss << "PUNTUACION  -  " << formatScore(minecraft->player->getScore());
	drawCenteredString(font, ss.str(), width / 2, baseY - 18, 0xEF4444);

    // Separador (línea sutil)
    fillGradient(width / 2 - 90, baseY - 8, width / 2 + 90, baseY - 7, 0x2AFFFFFF, 0x2AFFFFFF);

    glPopMatrix2(); // Fin del bloque animado

    // Textos informativos de espera y controles (siempre anclados al fondo de la pantalla)
    if (_tick < WAIT_TICKS) {
        drawCenteredString(font, "Espera un momento...", width / 2, height - 20, 0xBDB3B3);
    } else {
        drawCenteredString(font, "Enter para reaparecer  -  Esc para volver al menu", width / 2, height - 20, 0xBDB3B3);
    }

    // Dibujar botones
	Screen::render(xm, ym, a);
}

void DeathScreen::buttonClicked( Button* button )
{
	if (_tick < WAIT_TICKS) return;

	if (button == bRespawn) {
		minecraft->player->respawn();
		minecraft->setScreen(NULL);
	}

	if (button == bTitle)
		minecraft->leaveGame();
}