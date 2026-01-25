/**
 * @file VEDirectSerialReader.h
 * @brief Reads the latest message from a VE.Direct device via Serial,
 *        verifies the message checksum,
 *        and returns the message as a String
 *
 * @note The Serial buffer is read as-is. The latest message block is determined by
 *       locating an occurrence of "\r\nPID" followed by "Checksum".
 */


#pragma once
#include <Arduino.h>
#include <HardwareSerial.h>

class VEDirectSerialReader
{
private:
    HardwareSerial &serial;

    static constexpr size_t BUFFER_SIZE = 512;
    char ringBuffer[BUFFER_SIZE] = {0};
    size_t RingBufferWritePos = 0;

    char *lastMessage;
    size_t lastMessageLength = 0;

    const char *endOfMessage = "Checksum\t";

    void copySerialBufferToRingBuffer();

    int searchChecksumBackwards(const char *buffer, int head, const char *word);

    char *messageToLinearBuffer(const char *ringBuffer, size_t start, size_t end);

    bool verifyChecksum(const char *message, size_t length);

public:
    explicit VEDirectSerialReader(HardwareSerial &serialPort)
        : serial(serialPort) {}
    ~VEDirectSerialReader();
    bool update();
    String getMessage() const;
};
