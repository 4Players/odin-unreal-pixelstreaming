/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#include "OdinPixelStreamingAudioGenerator.h"
#include "IPixelStreamingModule.h"
#include "OdinPixelStreaming.h"
#include "OdinSubsystem.h"
#include "PixelStreamingAudioComponent.h"
#include "DSP/FloatArrayMath.h"

UOdinPixelStreamingAudioGenerator::UOdinPixelStreamingAudioGenerator()
    : Super()
      , AudioSink(nullptr)
      , bIsGenerating(false) {}

void UOdinPixelStreamingAudioGenerator::BeginDestroy()
{
    Super::BeginDestroy();
    StopGenerating();
}

bool UOdinPixelStreamingAudioGenerator::StartGenerating(FString PlayerToListenTo)
{
    IPixelStreamingModule& PixelStreamingModule = IPixelStreamingModule::Get();
    if (!PixelStreamingModule.IsReady()) {
        UE_LOG(LogOdinPixelStreaming, Error,
               TEXT("UOdinPixelStreamingAudioGenerator::StartListening PixelStreamingModule not "
                   "found, aborting."))
        return false;
    }
    return StreamerStartGenerating(PixelStreamingModule.GetDefaultStreamerID(), PlayerToListenTo);
}

bool UOdinPixelStreamingAudioGenerator::StreamerStartGenerating(FString StreamerId,
                                                                FString PlayerToListenTo)
{
    // Init WebRTC AudioSink
    if (!IPixelStreamingModule::IsAvailable()) {
        UE_LOG(LogOdinPixelStreaming, Verbose,
               TEXT("UOdinPixelStreamingAudioGenerator::StreamerStartListening Could not listen to "
                   "anything because Pixel Streaming module is not loaded. This is expected on "
                   "dedicated servers."));
        return false;
    }

    IPixelStreamingModule& PixelStreamingModule = IPixelStreamingModule::Get();
    if (!PixelStreamingModule.IsReady()) {
        UE_LOG(LogOdinPixelStreaming, Error,
               TEXT("UOdinPixelStreamingAudioGenerator::StreamerStartListening Pixel Streaming "
                   "Module is not ready, cannot Start Generating."));
        return false;
    }

    CurrentPlayerId = PlayerToListenTo;
    if (FString() == StreamerId) {
        TArray<FString> StreamerIds = PixelStreamingModule.GetStreamerIds();
        if (StreamerIds.Num() > 0) {
            CurrentStreamerId = StreamerIds[0];
        } else {
            CurrentStreamerId = PixelStreamingModule.GetDefaultStreamerID();
        }
    } else {
        CurrentStreamerId = StreamerId;
    }

    TSharedPtr<IPixelStreamingStreamer> Streamer =
        PixelStreamingModule.FindStreamer(CurrentStreamerId);
    if (!Streamer) {
        UE_LOG(LogOdinPixelStreaming, Error,
               TEXT("UOdinPixelStreamingAudioGenerator::StreamerStartListening Did not find "
                   "Streamer for StreamerId %s, cannot start generating Audio."),
               *CurrentStreamerId);
        return false;
    }
    IPixelStreamingAudioSink* CandidateSink =
        WillListenToAnyPlayer()
            ? Streamer->GetUnlistenedAudioSink()
            : Streamer->GetPeerAudioSink(FPixelStreamingPlayerId(CurrentPlayerId));

    if (CandidateSink == nullptr) {
        UE_LOG(LogOdinPixelStreaming, Verbose,
               TEXT("UOdinPixelStreamingAudioGenerator::StreamerStartListening Did not find a Peer "
                   "Audio Sink for Player Id %s."),
               *CurrentPlayerId);
        return false;
    }

    AudioSink = CandidateSink;
    AudioSink->AddAudioConsumer(this);

    bool bGeneratorWasInitialized = false;
    if (const UWorld*                               World          = GetWorld()) {
        if (const UGameInstance*                    GameInstance   = World->GetGameInstance()) {
            if (UOdinSubsystem* OdinInitSystem =
                GameInstance->GetSubsystem<UOdinSubsystem>()) {
                int32 OdinSampleRate   = OdinInitSystem->GetSampleRate();
                int32 OdinChannelCount = OdinInitSystem->GetChannelCount();
                UE_LOG(LogOdinPixelStreaming, Verbose,
                       TEXT("Init Odin Pixel Streaming Audio Generator with Sample Rate %d and "
                           "Channel Count %d"),
                       OdinSampleRate, OdinChannelCount);
                // Init UAudioGenerator with Odin Channels and SampleRate
                Init(OdinSampleRate, OdinChannelCount);
                bGeneratorWasInitialized = true;
            }
        }
    }
    bIsGenerating = bGeneratorWasInitialized;
    if (!bGeneratorWasInitialized) {
        UE_LOG(LogOdinPixelStreaming, Error,
               TEXT("UOdinPixelStreamingAudioGenerator::StreamerStartGenerating: Could not "
                   "initialize Generator, "
                   "because Odin Initialization System is not available. "
                   "Aborting Audio Generation for Streamer Id %s Player Id %s"),
               *StreamerId, *PlayerToListenTo);
        return false;
    }
    return bIsGenerating;
}

void UOdinPixelStreamingAudioGenerator::StopGenerating()
{
    if (AudioSink) {
        AudioSink->RemoveAudioConsumer(this);
    }

    AudioSink     = nullptr;
    bIsGenerating = false;
    bIsMuted = false;
    VolumeMultiplier = 1.0f;
}

bool UOdinPixelStreamingAudioGenerator::IsGenerating() const
{
    return bIsGenerating;
}

FString UOdinPixelStreamingAudioGenerator::GetConnectedPlayerId() const
{
    return CurrentPlayerId;
}

FString UOdinPixelStreamingAudioGenerator::GetConnectedStreamerId() const
{
    return CurrentStreamerId;
}

void UOdinPixelStreamingAudioGenerator::ConsumeRawPCM(const int16_t* AudioData, int    InSampleRate,
                                                      size_t         NChannels, size_t NFrames)
{
    if (!IsGenerating()) {
        return;
    }

    if(GetIsMuted()) {
        return;
    }

    if (nullptr == AudioData) {
        UE_LOG(LogOdinPixelStreaming, Error, TEXT("Audio Data null."));
    }

    if (SampleRate != InSampleRate || NChannels != NumChannels) {
        Init(InSampleRate, NChannels);
    }

    int32 NumSamples = NChannels * NFrames;

    AudioBuffer.SetNum(NumSamples, EAllowShrinking::No);
    Audio::ArrayPcm16ToFloat(MakeArrayView(AudioData, NumSamples),
                             MakeArrayView(AudioBuffer.GetData(), NumSamples));
    for (int32 BufferIndex = 0; BufferIndex < AudioBuffer.Num(); ++BufferIndex) {
        AudioBuffer[BufferIndex] *= VolumeMultiplier;
    }
    
    OnGeneratedAudio(AudioBuffer.GetData(), NumSamples);

    UE_LOG(LogOdinPixelStreaming, VeryVerbose,
           TEXT("Called ConsumeRawPCM with InSampleRate %d, NChanncels %llu, NFrames %llu, Num "
               "Samples Generated: %d"),
           InSampleRate, NChannels, NFrames, NumSamples);
}

void UOdinPixelStreamingAudioGenerator::OnConsumerAdded()
{
    UE_LOG(LogOdinPixelStreaming, Verbose,
           TEXT("UOdinPixelStreamingAudioGenerator::OnConsumerAdded"));
}

void UOdinPixelStreamingAudioGenerator::OnConsumerRemoved()
{
    UE_LOG(LogOdinPixelStreaming, Verbose,
           TEXT("UOdinPixelStreamingAudioGenerator::OnConsumerRemoved"));
    StopGenerating();
}

bool UOdinPixelStreamingAudioGenerator::GetIsMuted() const
{
    return bIsMuted;
}

void UOdinPixelStreamingAudioGenerator::SetIsMuted(bool bNewIsMuted)
{
    bIsMuted = bNewIsMuted;
}

float UOdinPixelStreamingAudioGenerator::GetVolumeMultiplier() const
{
    return VolumeMultiplier;
}

void UOdinPixelStreamingAudioGenerator::SetVolumeMultiplier(float NewMultiplierValue)
{
    VolumeMultiplier = NewMultiplierValue;
}

bool UOdinPixelStreamingAudioGenerator::WillListenToAnyPlayer() const
{
    return CurrentPlayerId == FString();
}