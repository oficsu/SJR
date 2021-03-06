#include <vector>
#include <map>

#include <fstream>

#include <cstring>
#include <cmath>
#include <type_traits>


class SJR
{

    public:

        enum class Type : int
        {
            BOOL = 0,
            INT = 1,
            FLOAT = 2,
            STRING = 3,
            ARRAY = 4,
            OBJECT = 5,
        };

        void load(std::string_view filename);

        [[nodiscard]]
        bool save(std::string_view filename);

        template<class T>
        void setValue(T newValue);

        [[nodiscard]]
        Type getType() const;
        template<class T>
        [[nodiscard]]
        T getValue() const;

        [[nodiscard]]
        size_t getChildCount() const;
        [[nodiscard]]
        size_t getArraySize() const;

        SJR& operator[] (std::string_view nodeName);
        SJR& operator[] (size_t index);

    private:

        std::map<std::string, SJR> mapJson;
        std::vector<SJR> vectorJson;

        std::string value;
        Type type = Type::OBJECT;

        static void writeTabs(std::ofstream& file, size_t count);
        static void skipWhiteSpace(char*&file);

        void writeBool(std::ofstream& file);
        void writeInt(std::ofstream &file);
        void writeFloat(std::ofstream &file);
        void writeString(std::ofstream &file);
        void writeArray(std::ofstream &file);
        void writeObject(std::ofstream &file);

        void write(std::ofstream& file);

        [[nodiscard]]
        bool parseBool(char*& file);
        [[nodiscard]]
        bool parseNumber(char*&file);
        [[nodiscard]]
        bool parseString(char*& file);
        [[nodiscard]]
        bool parseArray(char*& file);
        [[nodiscard]]
        bool parseObject(char*& file);

        [[nodiscard]]
        bool parse(char*& file);
};

#ifdef SJR_IMPLEMENTATION


//      ====================      ====================
//      ====================PUBLIC====================
//      ====================      ====================


void SJR::load(std::string_view filename)
{
    std::ifstream file(filename.data());
    std::string str;

    if (!file.is_open())
    {
        throw std::runtime_error("File cannot be opened.");
    }

    file.seekg(0, std::ios::end);
    str.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    str.assign((std::istreambuf_iterator<char>(file)),{});

    char* fileData = str.data();

    if (!parse(fileData))
    {
        throw std::runtime_error("File doesn't correspong to json format file.");
    }

}


[[nodiscard]]
bool SJR::save(std::string_view filename)
{
    std::ofstream file(filename.data());

    if (!file.is_open())
    {
        return false;
    }

    write(file);

    file.close();

    return true;
}


template<class T>
void SJR::setValue(T newValue)
{
    if constexpr(std::is_same_v<T, bool>)
    {
        type = Type::BOOL;
        value = std::to_string(newValue);
    }

    if constexpr(std::is_same_v<T, int>)
    {
        type = Type::INT;
        value = std::to_string(newValue);
    }

    if constexpr(std::is_same_v<T, float>)
    {
        type = Type::FLOAT;
        value = std::to_string(newValue);
    }

    if constexpr (std::is_same_v<T, std::string>)
    {
        type = Type::STRING;
        value = newValue;
    }
}


[[nodiscard]]
SJR::Type SJR::getType() const
{
    return type;
}


template<class T>
[[nodiscard]]
T SJR::getValue() const
{
    if constexpr(std::is_same_v<T, bool> || std::is_same_v<T, int>)
    {
        return std::stoi(value);
    }

    if constexpr(std::is_same_v<T, float>)
    {
        return std::stof(value);
    }

    if constexpr(std::is_same_v<T, std::string>)
    {
        return value;
    }
}


[[nodiscard]]
size_t SJR::getChildCount() const
{
    return mapJson.size();
}


[[nodiscard]]
size_t SJR::getArraySize() const
{
    return vectorJson.size();
}


[[nodiscard]]
SJR& SJR::operator[] (std::string_view nodeName)
{
    auto it = mapJson.find(nodeName.data());

    if (it != mapJson.end())
    {
        return it->second;
    }
    else
    {
        return mapJson[nodeName.data()];
    }
}


//  Indexing starts with 0.
//
[[nodiscard]]
SJR& SJR::operator[] (size_t index)
{
    if (type != Type::ARRAY)
    {
        vectorJson.resize(index);
        type = Type::ARRAY;
    }

    if (index >= vectorJson.size())
    {
        vectorJson.resize(index + 1);
    }

    return vectorJson.at(index);
}


//      ====================       ====================
//      ====================PRIVATE====================
//      ====================       ====================


void SJR::writeTabs(std::ofstream& file, size_t count)
{
    for (size_t i = 0u; i < count; ++i)
    {
        file << '\t';
    }
}


void SJR::skipWhiteSpace(char*&file)
{
    while (isspace(*file))
    {
        ++file;
    }
}


void SJR::writeBool(std::ofstream& file)
{
    file.setf(std::ios_base::boolalpha);
    file << static_cast<bool>(std::stoi(value));
    file.unsetf(std::ios::boolalpha);
}


void SJR::writeInt(std::ofstream &file)
{
    file << std::stoi(value);
}


void SJR::writeFloat(std::ofstream &file)
{
    file << std::stof(value);
}


void SJR::writeString(std::ofstream &file)
{
    file << "\"" << value << "\"";
}


void SJR::writeArray(std::ofstream &file)
{
    file << '[';

    for (auto it = vectorJson.begin(); it != vectorJson.end(); ++it)
    {
        it->write(file);

        if (it != (--vectorJson.end()))
        {
            file << ',' << ' ';
        }
    }

    file << ']';
}


void SJR::writeObject(std::ofstream &file)
{
    if (!value.empty())
    {
        file << "\"" << value << "\"";
        file << ": ";
    }

    static int tabsCount;

    file << '\n';
    SJR::writeTabs(file, tabsCount);
    file << '{' << '\n';

    ++tabsCount;
    SJR::writeTabs(file, tabsCount);

    for (auto it = mapJson.begin(); it != mapJson.end(); ++it)
    {
        file << "\"" << it->first << "\"";
        file << ": ";

        it->second.write(file);

        if (it != (--mapJson.end()))
        {
            file << ", ";
            file << '\n';
            SJR::writeTabs(file, tabsCount);
        }
    }

    --tabsCount;

    file << '\n';

    SJR::writeTabs(file, tabsCount);

    file << '}';

}


void SJR::write(std::ofstream& file)
{
    switch (type)
    {
        case Type::BOOL :
            writeBool(file);
            break;

        case Type::INT:
            writeInt(file);
            break;

        case Type::FLOAT :
            writeFloat(file);
            break;

        case Type::STRING:
            writeString(file);
            break;

        case Type::ARRAY:
            writeArray(file);
            break;

        case Type::OBJECT:
            writeObject(file);
            break;
    }


}


[[nodiscard]]
bool SJR::parseBool(char*& file)
{
    bool resultTrue = memcmp(file, "true", 4) == 0;
    bool resultFalse = memcmp(file, "false", 5) == 0;

    if (resultTrue || resultFalse)
    {
        value = std::to_string(resultTrue);
        file += resultTrue ? 4 : 5;
        type = Type::BOOL;
        return true;
    }

    return false;
}


[[nodiscard]]
bool SJR::parseNumber(char*& file)
{
    bool signNegative = *file == '-';
    bool signPositive = *file == '+';

    bool sign = signNegative | signPositive;

    int valueInt{0};
    float valueFloat{0.0f};

    if (sign || isdigit(*file))
    {
        if (sign)
        {
            ++file;
        }

        while (isdigit(*file))
        {
            valueInt *= 10;
            valueInt += *file - '0';
            ++file;
        }

        if (*file == '.')
        {
            ++file;

            type = Type::FLOAT;
            valueFloat = static_cast<float>(valueInt);

	    int shift = 1;
		
            while(isdigit(*file))
            {
                valueFloat *= pow(10, shift);
                valueFloat += static_cast<float>(*file - '0');
                valueFloat /= pow(10, shift);

		++shift;
                ++file;
            }

            value = std::to_string(valueFloat * (signNegative ? -1. : 1.));

            return true;
        }

        if (*file == 'e' || *file == 'E')
        {
            ++file;

            type = Type::FLOAT;
            valueFloat = static_cast<float>(valueInt);
            valueInt = 0;

            while (isdigit(*file))
            {
                valueInt *= 10;
                valueInt += *file - '0';
            }

            valueFloat *= static_cast<float>(pow(10, valueInt));

            value = std::to_string(valueFloat * (signNegative ? -1. : 1.));
            return true;
        }

        type = Type::INT;
        value = std::to_string(valueInt * (signNegative ? -1 : 1));
        return true;
    }

    return false;
}


[[nodiscard]]
bool SJR::parseString(char*& file)
{
    if (*file == '"')
    {
        ++file;

        type = Type::STRING;

        while (*file != '"')
        {
            SJR::skipWhiteSpace(file);

            if (*file == '"')
            {
                return true;
            }

            value += *file;

            ++file;
        }

        ++file;

        return true;
    }

    return false;
}


[[nodiscard]]
bool SJR::parseArray(char*& file)
{
    if (*file == '[')
    {
        ++file;

        type = Type::ARRAY;

        while (*file != ']')
        {
            SJR::skipWhiteSpace(file);

            if (*file == ']')
            {
                ++file;

                return true;
            }

            SJR newSJR;

            if (!newSJR.parse(file))
            {
                return false;
            }

            vectorJson.push_back(newSJR);

            SJR::skipWhiteSpace(file);

            if (*file == ']')
            {
                ++file;
                type = Type::ARRAY;
                return true;
            }

            if (*file != ',')
            {
                return false;
            }

            ++file;
        }

        ++file;
        return true;
    }

    return false;
}


[[nodiscard]]
bool SJR::parseObject(char*& file)
{
    if (*file == '{')
    {
        ++file;
        type = Type::OBJECT;

        while (*file != '}')
        {
            SJR::skipWhiteSpace(file);

            if (*file == '}')
            {
                ++file;
                return true;
            }


            if (*file == '"')
            {
                ++file;
                std::string nodeName{};

                while (*file != '"')
                {
                    SJR::skipWhiteSpace(file);

                    if (*file == '"')
                    {
                        break;
                    }

                    nodeName += *file;

                    ++file;
                }

                if (*file != '"')
                {
                    return true;
                }

                ++file;
                SJR::skipWhiteSpace(file);


                if (*file != ':')
                {
                    return true;
                }

                ++file;
                SJR::skipWhiteSpace(file);

                SJR newNode;
                if (!newNode.parse(file)) {
                    return true;
                }
                type = Type::OBJECT;
                mapJson[nodeName] = newNode;
            }

            SJR::skipWhiteSpace(file);

            if (*file == '}')
            {
                ++file;
                return true;
            }

            if (*file != ',')
            {
                return false;
            }

            ++file;
        }

        ++file;
        return true;
    }

    return false;
}


//
//
[[nodiscard]]
bool SJR::parse(char*& file)
{
    skipWhiteSpace(file);

    if (parseString(file))
    {
        return true;
    }

    if (parseBool(file))
    {
        return true;
    }

    if (parseNumber(file))
    {
        return true;
    }

    if (parseArray(file))
    {
        return true;
    }

    if (parseObject(file))
    {
        return true;
    }

    return false;
}


#endif
