#include"Animated.h"
#include<sys/stat.h>
#include<dirent.h>
#include<algorithm>
#include<fstream>

SDL_Renderer* Animated::rendererStatic = nullptr;

Animated::Animated() : currentFrame(0), lastFrameTime(0), elapsedTime(0.0f), isPlaying(false), isLooping(true), isGif(false) {
    srcRect = {0, 0, 0, 0};
    destRect = {0, 0, 0, 0};
}

Animated::Animated(SDL_Renderer* renderer, std::string filepath, bool looping) : Animated() {
    LoadAnimation(renderer, filepath, looping);
}

Animated::~Animated() {
    // 不再在析构函数中调用CleanUp()，因为它已经在Scene的Exit方法中被调用了
    // 这样可以避免双重清理导致的Segmentation fault
}

void Animated::AnimatedInit(SDL_Renderer* renderer) {
    rendererStatic = renderer;
}

// 检查路径是否为文件夹
bool isDirectory(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}

// 检查文件是否为GIF
bool isGifFile(const std::string& path) {
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos) {
        return false;
    }
    std::string ext = path.substr(pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == "gif";
}

// 加载文件夹中的所有图片
bool Animated::loadFromDirectory(SDL_Renderer* renderer, const std::string& dirPath) {
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) {
        return false;
    }

    std::vector<std::string> imageFiles;
    struct dirent* entry;
    
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) {
            std::string filename = entry->d_name;
            size_t pos = filename.find_last_of('.');
            if (pos != std::string::npos) {
                std::string ext = filename.substr(pos + 1);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp") {
                    imageFiles.push_back(dirPath + "/" + filename);
                }
            }
        }
    }
    closedir(dir);

    if (imageFiles.empty()) {
        return false;
    }

    // 对文件进行排序，确保按数字顺序排列
    std::sort(imageFiles.begin(), imageFiles.end(), [](const std::string& a, const std::string& b) {
        // 提取文件名（不含路径）
        std::string filenameA = a.substr(a.find_last_of('/') + 1);
        std::string filenameB = b.substr(b.find_last_of('/') + 1);
        
        // 提取数字部分
        size_t numStartA = 0;
        while (numStartA < filenameA.size() && !isdigit(filenameA[numStartA])) {
            numStartA++;
        }
        size_t numEndA = numStartA;
        while (numEndA < filenameA.size() && isdigit(filenameA[numEndA])) {
            numEndA++;
        }
        
        size_t numStartB = 0;
        while (numStartB < filenameB.size() && !isdigit(filenameB[numStartB])) {
            numStartB++;
        }
        size_t numEndB = numStartB;
        while (numEndB < filenameB.size() && isdigit(filenameB[numEndB])) {
            numEndB++;
        }
        
        // 转换为数字进行比较
        int numA = 0, numB = 0;
        if (numStartA < numEndA) {
            numA = std::stoi(filenameA.substr(numStartA, numEndA - numStartA));
        }
        if (numStartB < numEndB) {
            numB = std::stoi(filenameB.substr(numStartB, numEndB - numStartB));
        }
        
        return numA < numB;
    });

    // 加载所有图片作为帧
    for (const auto& file : imageFiles) {
        SDL_Surface* surface = IMG_Load(file.c_str());
        if (!surface) {
            continue;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
            SDL_FreeSurface(surface);
            continue;
        }

        frames.push_back(texture);
        frameDelays.push_back(100); // 默认每帧100毫秒
        SDL_FreeSurface(surface);
    }

    if (!frames.empty()) {
        // 设置第一帧的尺寸
        int w, h;
        SDL_QueryTexture(frames[0], nullptr, nullptr, &w, &h);
        srcRect = {0, 0, w, h};
        destRect = {0, 0, w, h};
        return true;
    }

    return false;
}

// 加载GIF文件（改为加载图集）
bool Animated::loadFromGif(SDL_Renderer* renderer, const std::string& gifPath) {
    // 这里我们改为从文件夹加载图集
    // 假设gifPath是一个文件夹路径，包含一系列图片
    std::string folderPath = gifPath;
    
    // 检查路径是否存在
    struct stat info;
    if (stat(folderPath.c_str(), &info) != 0) {
        return false;
    }
    
    if (!S_ISDIR(info.st_mode)) {
        // 如果不是文件夹，尝试从文件名中提取文件夹路径
        size_t lastSlash = folderPath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            folderPath = folderPath.substr(0, lastSlash);
        } else {
            return false;
        }
    }
    
    // 加载文件夹中的所有图片
    return loadFromDirectory(renderer, folderPath);
}

bool Animated::LoadAnimation(SDL_Renderer* renderer, std::string filepath, bool looping) {
    // 清理现有资源
    CleanUp();
    
    this->filepath = filepath;
    this->isLooping = looping;
    
    if (isDirectory(filepath)) {
        // 加载文件夹中的图片
        isGif = false;
        return loadFromDirectory(renderer, filepath);
    } else if (isGifFile(filepath)) {
        // 加载GIF文件
        isGif = true;
        return loadFromGif(renderer, filepath);
    } else {
        SDL_Log("Invalid path: %s. Must be a directory or a GIF file.", filepath.c_str());
        return false;
    }
}

void Animated::Play() {
    if (!frames.empty()) {
        isPlaying = true;
        lastFrameTime = SDL_GetTicks();
    }
}

void Animated::Pause() {
    isPlaying = false;
}

void Animated::Stop() {
    isPlaying = false;
    currentFrame = 0;
    lastFrameTime = 0;
}

void Animated::SetLooping(bool looping) {
    isLooping = looping;
}

void Animated::Update(float deltaTime) {
    if (!isPlaying || frames.empty()) {
        return;
    }

    // 累积时间
    elapsedTime += deltaTime * 1000; // 转换为毫秒
    
    if (elapsedTime >= frameDelays[currentFrame]) {
        elapsedTime = 0.0f;
        currentFrame++;
        
        // 确保帧索引在有效范围内
        if (currentFrame >= frames.size()) {
            if (isLooping) {
                currentFrame = 0;
            } else {
                currentFrame = frames.size() - 1;
                isPlaying = false;
            }
        }
    }
}

void Animated::Render(SDL_Renderer* renderer, SDL_Rect* destRect) {
    if (frames.empty()) {
        return;
    }

    SDL_Rect* renderRect = destRect ? destRect : &this->destRect;
    SDL_RenderCopy(renderer, frames[currentFrame], &srcRect, renderRect);
}

int Animated::GetFrameCount() const {
    return frames.size();
}

int Animated::GetCurrentFrameIndex() const {
    return currentFrame;
}

bool Animated::IsPlaying() const {
    return isPlaying;
}

bool Animated::IsLooping() const {
    return isLooping;
}

void Animated::CleanUp() {
    for (auto texture : frames) {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
    }
    frames.clear();
    frameDelays.clear();
    currentFrame = 0;
    lastFrameTime = 0;
    elapsedTime = 0.0f;
    isPlaying = false;
    isLooping = true;
    isGif = false;
}
