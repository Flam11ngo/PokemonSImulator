#include"GUIUtils/Button.h"

void Button::SetCallback(std::function<void()> callback) {
	this->callback = std::move(callback);
}

void Button::HandleEvents(SDL_Event& event) {
	int x, y;
	SDL_GetMouseState(&x, &y);
	SDL_Point mousePoint{ x,y };
	if (SDL_PointInRect(&mousePoint, getDrect()) ==SDL_bool::SDL_FALSE) {
		
		ButtonState = Normal;
	}
	else {
		switch (event.type) {
		case SDL_MOUSEMOTION:
			ButtonState = Hover;
			break;
		case SDL_MOUSEBUTTONDOWN:
			ButtonState = Pressed;
			break;
		case SDL_MOUSEBUTTONUP:
			if (ButtonState == Pressed && callback) {
				ButtonState = Hover;
				callback();
			}
			break;
		}
	}
}

void Button::Update(const float deltaTime) {
	// Update logic if needed
}

void Button::Render(SDL_Renderer* render) {
	// Render logic based on Buttonstate
	SDL_RenderCopy(render, background.getTexture(), nullptr, getDrect());
	switch (ButtonState) {
	case Normal:
		SDL_SetRenderDrawColor(render, 255, 0, 0, 255);
		SDL_RenderDrawRect(render, getDrect());
		break;
	case Hover:
		SDL_SetRenderDrawColor(render, 0, 255, 0, 255);
		SDL_RenderDrawRect(render, getDrect());
		break;
	case Pressed:
		SDL_SetRenderDrawColor(render, 0, 0, 255, 255);
		SDL_RenderDrawRect(render, getDrect());
		break;
	}
}