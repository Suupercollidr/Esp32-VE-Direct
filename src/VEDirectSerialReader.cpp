#include "VEDirectSerialReader.h"

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
    if (start >= BUFFER_SIZE || end >= BUFFER_SIZE) // Hantera fel, t.ex. returnera nullptr eller kasta ett undantag
    {
        Serial.println("Start- eller slutposition utanför bufferten");
        return nullptr;
    }
    // Beräkna längden på meddelandet
    size_t messageLength;
    if (end > start)
        messageLength = end - start;
    else
        messageLength = (BUFFER_SIZE - start) + end; // Meddelandet sträcker sig över buffertens slut

    Serial.println("Meddelandet är " + String(messageLength) + " tecken långt");

    // Allokera en ny buffer för meddelandet (+1 för \0 om du vill)
    char *messageBuffer = new char[messageLength + 1];

    // Kopiera meddelandet till den nya bufferten
    if (end > start)
    {
        // Meddelandet är linjärt i bufferten
        memcpy(messageBuffer, &ringBuffer[start], messageLength);
    }
    else
    {
        // Meddelandet sträcker sig över buffertens slut
        size_t firstPartLength = BUFFER_SIZE - start;
        memcpy(messageBuffer, &ringBuffer[start], firstPartLength);
        memcpy(messageBuffer + firstPartLength, ringBuffer, end);
    }

    //    messageBuffer[messageLength] = '\0';

    Serial.println();
    for (int i = 0; i < messageLength; i++)
    {
        char j = messageBuffer[i];
        Serial.print(j);
    }
    Serial.println();
    Serial.print("Första tecknet är: ");
    Serial.println(static_cast<int>(messageBuffer[0]));
    Serial.print("Sista tecknet är: ");
    Serial.println(static_cast<int>(messageBuffer[messageLength - 1]));
    Serial.flush();

    return messageBuffer;
}

bool VEDirectSerialReader::verifyChecksum(const char *message, size_t length)
{
    if (length < 1)
        return false;

    uint8_t sum = 0;
    Serial.println("Meddelandet är " + String(length) + " tecken långt.");

    for (size_t i = 0; i < length; i++)
    {
        uint8_t thisByte = static_cast<uint8_t>(message[i]); // säker på binär data
        sum += thisByte;                                     // summera korrekt
        // Serial.println(String(sum));
        // Serial.printf("%d, ", thisByte);                     // skriv ut som tal
    }

    Serial.println();
    Serial.println("Summa inkl. checksum (ska vara 0): " + String(sum));
    Serial.print("Checksum i meddelandet: ");
    Serial.println(static_cast<uint8_t>(message[length - 1]));

    return (sum == 0);
}

bool VEDirectSerialReader::update()
{

    if (!serial.available())
    {
        Serial.print(".");
        return false;
    }
    size_t start, end, length;

    copySerialBufferToRingBuffer();

    int endPos = searchChecksumBackwards(ringBuffer, RingBufferWritePos, endOfMessage);
    if (endPos == -1)
    {
        Serial.println("Hittade inget meddelandeslut");
        Serial.flush();
        return false;
    }
    end = (endPos + strlen(endOfMessage) + 1) % BUFFER_SIZE;

    int searchStartPos = (endPos - 1 + BUFFER_SIZE) % BUFFER_SIZE;
    int startPos = searchChecksumBackwards(ringBuffer, searchStartPos, endOfMessage);

    if (startPos < 0 || startPos == endPos) // If only find one endOfMessage, assume there is exactly 1 message at the start of the buffer
    {
        Serial.println("Hittade inget gammalt meddelande, så börjar på 0");
        Serial.flush();
        start = 0;
    }
    else
    {
        start = (startPos + strlen(endOfMessage) + 1) % BUFFER_SIZE;
    }
    Serial.println("Meddelandet är från " + String(start) + " till " + String(end));

    char *message = messageToLinearBuffer(ringBuffer, start, end);
    if (message == nullptr)
        return false;

    length = (end >= start)
                 ? (end - start)
                 : (BUFFER_SIZE - start + end);

    if (!verifyChecksum(message, length))
    {
        Serial.println("Felaktig checksum");
        return false;
    }

    if (lastMessage != nullptr)
        delete[] lastMessage;
    lastMessage = message;

    lastMessageLength = length;

    return true;
}

String VEDirectSerialReader::getMessage() const
{
    if (lastMessage == nullptr)
        return String("");
    String strMessage = String(lastMessage, lastMessageLength);
    return strMessage;
}
