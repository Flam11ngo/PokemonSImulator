#include"Image.h"
#include "SDL_rect.h"
#include "SDL_render.h"

SDL_Renderer* Image::rendererStatic = nullptr;

Image::Image(SDL_Renderer* renderer, std::string filepath) {
	this->filepath = filepath;
	LoadImage(renderer, filepath);
}

Image::Image(const Image& other) {
    if (this == &other) return;
    // 先释放当前资源
    CleanUp();
    // 拷贝文件路径
    filepath = other.filepath;

    // 深度拷贝 SDL_Surface
    if (other.surface) {
        // SDL_ConvertSurface 创建 surface 的深度拷贝
        surface = SDL_ConvertSurface(other.surface,
            other.surface->format,
            other.surface->flags);
		if (surface == nullptr) {
			SDL_Log("Failed to convert surface");
		}
        // 从深度拷贝的 surface 创建新的 texture
        if (rendererStatic && surface) {
            texture = SDL_CreateTextureFromSurface(rendererStatic, surface);
            if (!texture) {
                SDL_FreeSurface(surface);
                surface = nullptr;
				SDL_Log("Failed to create texture");
            }

            // 初始化 srcRect
			if (surface != nullptr) {
				srcRect.w = surface->w;
				srcRect.h = surface->h;
			}
        }
    }
}

Image& Image::operator=(const Image& other) {
    if (this == &other) return *this;

    // 先释放当前资源
    CleanUp();
    // 拷贝文件路径
    filepath = other.filepath;

    // 深度拷贝 SDL_Surface
    if (other.surface) {
        surface = SDL_ConvertSurface(other.surface,
            other.surface->format,
            other.surface->flags);
        if (!surface) {
			SDL_Log("Failed to convert surface");
        }

        // 创建新的 texture
        if (rendererStatic && surface) {
            texture = SDL_CreateTextureFromSurface(rendererStatic, surface);
            if (!texture) {
                SDL_FreeSurface(surface);
                surface = nullptr;
                SDL_Log("Failed to create texture");
            }

            // 初始化 srcRect
            if (surface != nullptr) {
                srcRect.w = surface->w;
                srcRect.h = surface->h;
            }
        }
    }

    return *this;
}

Image::Image(Image&& other) noexcept
	: surface(other.surface), texture(other.texture),
	srcRect(other.srcRect), filepath(std::move(other.filepath)) {
	other.surface = nullptr;
	other.texture = nullptr;
	other.srcRect = { 0,0,0,0 };
	other.filepath.clear();
}

Image::~Image() {
    CleanUp();
}

void Image::ImgInit(SDL_Renderer *renderer) {
	rendererStatic = renderer;
}

void Image::CleanUp() {
    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
    if (surface != nullptr) {
        SDL_FreeSurface(surface);
        surface = nullptr;
    }
}

void Image::LoadImage(SDL_Renderer* renderer, std::string filepath){
	surface = IMG_Load(filepath.c_str());
	if (surface == nullptr) {
		SDL_Log("Failed to load image: %s, SDL_image Error: %s", filepath.c_str(), IMG_GetError());
		return;
	}
	texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (texture == nullptr) {
		SDL_Log("Failed to create texture from surface: %s, SDL Error: %s", filepath.c_str(), SDL_GetError());
		SDL_FreeSurface(surface);
		surface = nullptr;
		return;
	}
	SDL_Log("Image Loaded: %s", filepath.c_str());
	if (surface != nullptr) {
		srcRect.w = surface->w;
		srcRect.h = surface->h;
	}
}

void Image::Render(SDL_Renderer* renderer,SDL_Rect*destRect) {
	destRect = destRect ? destRect : const_cast<SDL_Rect*>(getDestRect());
	SDL_RenderCopy(renderer, texture,getSrcRect(), destRect);
}