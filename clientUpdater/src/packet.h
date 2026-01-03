#pragma once

#include <vector>     
#include <cstdint>     
#include <string>    
#include <stdexcept>   

namespace callifornia {

class Packet {
private:
    struct PacketHeader {
        int type = 0;
        int64_t size = 0;
    };

public:
    Packet();
    Packet(int type, const std::string& str);
    void setData(const std::string& str);
    std::string data();
    void clear();
    uint32_t type() const;
    void setType(int type);

private:
    friend class NetworkController;

    uint32_t size() const;
    static size_t sizeOfHeader();

    const PacketHeader& header() const;
    PacketHeader& header_mut();

    const std::vector<uint8_t>& body() const;
    std::vector<uint8_t>& body_mut();

    size_t calculateSize() const;
    void putSize(uint32_t value);
    void extractSize(uint32_t& value);

private:
    PacketHeader m_header{};
	std::vector<uint8_t> m_body{};
	};

	}
}
