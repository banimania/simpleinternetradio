#include "client.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <portaudio.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 256
#define NUM_CHANNELS 2
#define BUFFER_SIZE (FRAMES_PER_BUFFER * NUM_CHANNELS * sizeof(float))

struct AudioData {
  int socket;
};

static int playCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
  AudioData *data = (AudioData*)userData;
  int socket = data->socket;

  ssize_t bytesReceived = recv(socket, outputBuffer, BUFFER_SIZE, 0);
  if (bytesReceived <= 0) {
    return paComplete;
  }

  return paContinue;
}


void sendChatMessage(int socket, const std::string &message) {
  send(socket, message.c_str(), message.size(), 0);
}

Client::Client(std::string ip, int port) {
  PaError err = Pa_Initialize();
  if (err != paNoError) {
    std::cout << "ERROR: " << Pa_GetErrorText(err) << std::endl;
    exit(-1);
  }

  PaStream* stream;

  PaStreamParameters outputParameters;
  outputParameters.device = Pa_GetDefaultOutputDevice();
  if (outputParameters.device == paNoDevice) {
    std::cout << "No default output device available" << std::endl;
    Pa_Terminate();
    exit(-1);
  }
  outputParameters.channelCount = NUM_CHANNELS;
  outputParameters.sampleFormat = paFloat32;
  outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
  outputParameters.hostApiSpecificStreamInfo = nullptr;

  client_socket = socket(AF_INET, SOCK_STREAM, 0);

  sockaddr_in server_address; 
  server_address.sin_family = AF_INET; 
  server_address.sin_port = htons(port); 
  server_address.sin_addr.s_addr = inet_addr(ip.c_str()); 

  if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
    std::cout << "ERROR: Could not connect to server" << std::endl;
    exit(-1);
  }

  AudioData audioData;
  audioData.socket = client_socket;

  err = Pa_OpenStream(&stream, nullptr, &outputParameters, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, playCallback, &audioData);
  if (err != paNoError) {
    std::cout << "ERROR: " << Pa_GetErrorText(err) << std::endl;
    Pa_Terminate();
    close(client_socket);
    exit(-1);
  }

  err = Pa_StartStream(stream);
  if (err != paNoError) {
    std::cout << "ERROR: " << Pa_GetErrorText(err) << std::endl;
    Pa_CloseStream(stream);
    Pa_Terminate();
    close(client_socket);
    exit(-1);
  }

  std::cout << "Playing audio..." << std::endl;

  while (Pa_IsStreamActive(stream)) {
    Pa_Sleep(1000);
  }

  Pa_CloseStream(stream);
  Pa_Terminate();
  close(client_socket);
}
