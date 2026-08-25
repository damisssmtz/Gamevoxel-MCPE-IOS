#include <iostream>
#include <string>
#include <vector>

std::string getJsonProp(const std::string& json, const std::string& prop) {
    size_t p = json.find("\"" + prop + "\"");
    if (p == std::string::npos) return "";
    size_t colon = json.find(":", p);
    if (colon == std::string::npos) return "";
    size_t q1 = json.find("\"", colon);
    if (q1 == std::string::npos) return "";
    size_t q2 = json.find("\"", q1 + 1);
    if (q2 == std::string::npos) return "";
    return json.substr(q1 + 1, q2 - q1 - 1);
}

int main() {
    std::string content = "{\"skins\":[{\"localization_name\":\"Kakashi\",\"geometry\":\"geometry.Kakashi.Kakashi\",\"texture\":\"skin_R26XWFR2.png\",\"type\":\"free\"}],\"serialize_name\":\"Kakashi\",\"localization_name\":\"Kakashi\"}";
    size_t skinsArrayPos = content.find("\"skins\"");
    size_t skinsArrayEnd = content.find(']', skinsArrayPos);
    
    size_t pos = skinsArrayPos;
    while ((pos = content.find('{', pos)) != std::string::npos && pos < skinsArrayEnd) {
        size_t endObj = content.find('}', pos);
        if (endObj == std::string::npos || endObj > skinsArrayEnd) break;
        std::string objStr = content.substr(pos, endObj - pos + 1);
        std::cout << "Found obj: " << objStr << std::endl;
        std::string texture = getJsonProp(objStr, "texture");
        std::cout << "Found texture: " << texture << std::endl;
        pos = endObj + 1;
    }
    return 0;
}
