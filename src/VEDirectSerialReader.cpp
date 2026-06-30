#include "VEDirectSerialReader.h"
#include "EventLogger.h"

extern EventLogger eventLog;

VEDirectSerialReader::~VEDirectSerialReader()
{
    if (lastMessage != nullptr)
    {
        delete[] lastMessage; // Frigör det allokerade minnet
        lastMessage = nullptr;
    }
}

/**
 * @brief Copy all chars available in the serial buffer to a ring buffer
 *
 */
void VEDirectSerialReader::copySerialBufferToRingBuffer()
{
    while (serial.available() > 0)
    {
        ringBuffer[RingBufferWritePos++] = serial.read();

        if (RingBufferWritePos >= BUFFER_SIZE)
            RingBufferWritePos = 0;

        if (!serial.available())
            delay(1); // Give device a chance to finish
    }
}

int VEDirectSerialReader::searchChecksumBackwards(const char *buffer, int head, const char *word)
{
    const int bufferSize = BUFFER_SIZE;
    const int wordLength = strlen(word);
    int current = (head - wordLength + bufferSize) % bufferSize; // Börja från senast skrivna positionen

    for (int i = 0; i < bufferSize; ++i)
    {
        // Kolla om det finns tillräckligt med tecken kvar för att matcha 'word' + 1
        bool match = true;
        for (int j = 0; j < wordLength; ++j)
        {
            int index = (current + j + bufferSize) % bufferSize;
            if (buffer[index] != word[j])
            {
                match = false;
                break;
            }
        }
        if (match)
            return current; // Returnera positionen om match hittas
        // Gå baklänges (cirkulärt)
        current = (current - 1 + bufferSize) % bufferSize;
    }

    Serial.println("Hittade inte \"" + String(word) + "\"");
    return -1; // Returnera -1 om ingen match hittas
}

char *VEDirectSerialReader::messageToLinearBuffer(const char *ringBuffer, size_t start, size_t end)
{
    if (start >= BUFFER_SIZE || end >= BUFFER_SIZE)
    {
        eventLog.log("Start- eller slutposition utanför bufferten", EventLogger::LogLevel::DATA);
        return nullptr;
    }

    // Beräkna längden på meddelandet
    size_t messageLength;
    if (end > start)
        messageLength = end - start;
    else
        messageLength = (BUFFER_SIZE - start) + end; // Meddelandet sträcker sig över buffertens slut

    // Allokera en ny buffer för meddelandet (+1 för \0 om du vill)
    char *messageBuffer = new char[messageLength + 1];

    // Kopiera meddelandet till den nya bufferten
    if (end > start) // Meddelandet är linjärt i bufferten
        memcpy(messageBuffer, &ringBuffer[start], messageLength);

    else // Meddelandet sträcker sig över buffertens slut
    {
        size_t firstPartLength = BUFFER_SIZE - start;
        memcpy(messageBuffer, &ringBuffer[start], firstPartLength);
        memcpy(messageBuffer + firstPartLength, ringBuffer, end);
    }

    return messageBuffer;
}

bool VEDirectSerialReader::verifyChecksum(const char *message, size_t length)
{
    if (length < 1)
        return false;

    uint8_t sum = 0;

    for (size_t i = 0; i < length; i++)
    {
        uint8_t thisByte = static_cast<uint8_t>(message[i]);
        sum += thisByte;
    }

    return (sum == 0);
}

bool VEDirectSerialReader::update()
{

    if (!serial.available())
        return false;

    size_t start, end, length;

    copySerialBufferToRingBuffer();

    int endPos = searchChecksumBackwards(ringBuffer, RingBufferWritePos, endOfMessage);
    if (endPos == -1)
    {
        Serial.println("Hittar inget meddelandeslut");
        Serial.flush();
        return false;
    }
    end = (endPos + strlen(endOfMessage) + 1) % BUFFER_SIZE;

    int searchStartPos = (endPos - 1 + BUFFER_SIZE) % BUFFER_SIZE;
    int startPos = searchChecksumBackwards(ringBuffer, searchStartPos, endOfMessage);

    if (startPos < 0 || startPos == endPos) // If only find one endOfMessage, assume there is exactly 1 message at the start of the buffer
        start = 0;
    else
        start = (startPos + strlen(endOfMessage) + 1) % BUFFER_SIZE;

    char *message = messageToLinearBuffer(ringBuffer, start, end);

    if (message == nullptr)
        return false;

    length = (end >= start)
                 ? (end - start)
                 : (BUFFER_SIZE - start + end);

    // Filtrera bort HEX-rader
    size_t filteredLength = filterHexLines(message, length);
    if (filteredLength == 0)     // Inget kvar efter filtrering
    {
        delete[] message;
        return false;
    }

    // Uppdatera längden
    length = filteredLength;

    if (!verifyChecksum(message, length))
    {
        eventLog.log("Felaktig kontrollsumma för:\n" + String(message), EventLogger::LogLevel::DATA);
        delete[] message;
        return false;
    }

    if (lastMessage != nullptr)
        delete[] lastMessage;
    lastMessage = message;

    lastMessageLength = length;

    return true;
}

size_t VEDirectSerialReader::filterHexLines(char *buffer, size_t length)
{
    size_t writePos = 0;
    bool inHexLine = false;

    for (size_t readPos = 0; readPos < length; readPos++)
    {
        if (buffer[readPos] == ':')
            inHexLine = true; // Början på en HEX-rad

        if (!inHexLine)
            buffer[writePos++] = buffer[readPos]; // Kopiera tecknet om vi inte är i en HEX-rad

        if (inHexLine && (buffer[readPos] == '\n' || buffer[readPos] == '\r'))
            inHexLine = false; // Slut på HEX-rad
    }

    return writePos; // Returnera den nya längden på den filtrerade bufferten
}

String VEDirectSerialReader::getMessage() const
{
    if (lastMessage == nullptr)
        return String("");
    String strMessage = String(lastMessage, lastMessageLength);
    return strMessage;
}
