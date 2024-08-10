#include "server.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <portaudio.h>
#include <vector>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 256
#define NUM_CHANNELS 2
#define BUFFER_SIZE (FRAMES_PER_BUFFER * NUM_CHANNELS * sizeof(float))
#define BUFFER_CHAT_SIZE 1024

struct ClientData {
  int socket;
  bool connected;
};

std::vector<ClientData> clients;

static int recordCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
  if (inputBuffer != nullptr) {
    for (ClientData& client : clients) {
      if (client.connected) {
        ssize_t bytesSent = send(client.socket, inputBuffer, BUFFER_SIZE, 0);
        if (bytesSent <= 0) {
          client.connected = false;
        }
      }
    }
  }
  
  return paContinue;
}

void handleClient(int clientSocket) {
  ClientData clientData = {clientSocket, true};

  clients.push_back(clientData);
 
  while (clientData.connected) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  close(clientSocket);
  
  clients.erase(std::remove_if(clients.begin(), clients.end(), [&](ClientData& c) {
    return c.socket == clientSocket;
  }), clients.end());
}


Server::Server(int port) {

  PaError err = Pa_Initialize();
  if (err != paNoError) {
    std::cout << "ERROR: " << Pa_GetErrorText(err) << std::endl;
    exit(-1);
  }

  PaStream* stream;
  PaStreamParameters inputParameters;
  inputParameters.device = Pa_GetDefaultInputDevice();
  if (inputParameters.device == paNoDevice) {
    std::cerr << "ERROR: No default input device available" << std::endl;
    Pa_Terminate();
    exit(-1);
  }
  inputParameters.channelCount = NUM_CHANNELS;
  inputParameters.sampleFormat = paFloat32;
  inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
  inputParameters.hostApiSpecificStreamInfo = nullptr;

  if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    std::cout << "ERROR: Could not create socket" << std::endl;
    exit(-1);
  };

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  server_address.sin_addr.s_addr = INADDR_ANY;
  
  int server_address_len = sizeof(server_address);

  int opt = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    std::cout << "ERROR: Could not set socket options" << std::endl;
    exit(-1);
  }
  
  if (bind(server_socket, (struct sockaddr*)& server_address, server_address_len) == -1) {
    std::cout << "ERROR: Could not bind socket" << std::endl;
    exit(-1);
  }

  if (listen(server_socket, 128) == -1) {
    std::cout << "ERROR: Could not listen to connections" << std::endl;
    exit(-1);
  }

  std::cout << "Server is listening in port " << port << std::endl;

  err = Pa_OpenStream(&stream, &inputParameters, nullptr, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, recordCallback, nullptr);
  if (err != paNoError) {
    std::cout << "ERROR: " << Pa_GetErrorText(err) << std::endl;
    Pa_Terminate();
    close(server_socket);
    exit(-1);
  }

  err = Pa_StartStream(stream);
  if (err != paNoError) {
    std::cout << "ERROR: " << Pa_GetErrorText(err) << std::endl;
    Pa_CloseStream(stream);
    Pa_Terminate();
    close(server_socket);
    exit(-1);
  }

  std::cout << "Streaming audio..." << std::endl;
  while (true) {
    int new_client_socket;
    if ((new_client_socket = accept(server_socket, (struct sockaddr *)&server_address, (socklen_t *)&server_address_len)) < 0) {
      std::cout << "ERROR: Could not accept client" << std::endl;
      continue;
    }
    std::thread clientThread(handleClient, new_client_socket);
    clientThread.detach();
  }

  Pa_StopStream(stream);
  Pa_CloseStream(stream);
  Pa_Terminate();
  close(server_socket);
}
