#include "packet.h"
#include <cstring>

namespace callifornia {
	namespace updater {

Packet::Packet() {
    clear();
}

Packet::Packet(int type, const std::string& str) {
    setData(str);
    m_header.type = type;
}

void Packet::setData(const std::string& str) {
    m_header.size = 0;
    m_body.clear();

    m_body.insert(m_body.end(), str.begin(), str.end());

    uint32_t size = static_cast<uint32_t>(str.size());
    putSize(size);

    m_header.size = this->size();
}

std::string Packet::data() {
    uint32_t size = 0;
    extractSize(size);

    if (m_body.size() < size)
        throw std::runtime_error("Packet body too small for string");

    std::string str;
    str.assign(reinterpret_cast<const char*>(m_body.data()), size);
    m_body.erase(m_body.begin(), m_body.begin() + size);
    m_header.size = this->size();

    return str;
}


void Packet::clear() {
    m_header.type = 0;
    m_header.size = 0;
    m_body.clear();
}

void Packet::setType(int type) {
    m_header.type = type;
}

uint32_t Packet::type() const {
    return m_header.type;
}

const Packet::PacketHeader& Packet::header() const {
    return m_header;
}

Packet::PacketHeader& Packet::header_mut() {
    return m_header;
}

const std::vector<uint8_t>& Packet::body() const {
    return m_body;
}

std::vector<uint8_t>& Packet::body_mut() {
    return m_body;
}

size_t Packet::calculateSize() const {
    return sizeof(PacketHeader) + m_body.size();
}

void Packet::putSize(uint32_t value) {
    size_t i = m_body.size();
    m_body.resize(m_body.size() + sizeof(uint32_t));
    memcpy(m_body.data() + i, &value, sizeof(uint32_t));
    m_header.size = this->calculateSize();
}

void Packet::extractSize(uint32_t& value) {
    if (m_body.size() < sizeof(uint32_t))
        throw std::runtime_error("Packet body too small for uint32_t");

    size_t i = m_body.size() - sizeof(uint32_t);
    memcpy(&value, m_body.data() + i, sizeof(uint32_t));
    m_body.resize(i);
    m_header.size = this->calculateSize();
}

uint32_t Packet::size() const {
    return m_header.size;
}

size_t Packet::sizeOfHeader() {
	return sizeof(PacketHeader);
}

	}
}
