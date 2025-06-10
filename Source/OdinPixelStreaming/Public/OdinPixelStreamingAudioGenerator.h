/* Copyright (c) 2022-2024 4Players GmbH. All rights reserved. */

#pragma once

#include "CoreMinimal.h"
#include "IPixelStreamingAudioConsumer.h"
#include "OdinAudioControl.h"
#include "Generators/AudioGenerator.h"

#include "OdinPixelStreamingAudioGenerator.generated.h"

class IPixelStreamingAudioSink;
class FWebRTCSoundGenerator;
/**
 * Audio Generator class for integrating 4Players ODIN technology with Unreal's PixelStreaming.
 * This class consumes raw PCM audio from PixelStreaming and generates audio output.
 */
UCLASS(Blueprintable, BlueprintType)
class ODINPIXELSTREAMING_API UOdinPixelStreamingAudioGenerator : public UAudioGenerator,
                                                                 public
                                                                 IPixelStreamingAudioConsumer,
                                                                 public IOdinAudioControl
{
    GENERATED_BODY()

protected:
    UOdinPixelStreamingAudioGenerator();

    virtual void BeginDestroy() override;

public:
    /**
     * Starts generating audio for a specific player ID.
     * @param PlayerToListenTo - The ID of the player to listen to.
     * @return True if successfully started, false otherwise.
     */
    UFUNCTION(BlueprintCallable)
    bool StartGenerating(FString PlayerToListenTo);
    /**
     * Starts generating audio for a specific streamer and player ID.
     * @param StreamerId - The ID of the streamer to associate with.
     * @param PlayerToListenTo - The ID of the player to listen to.
     * @return True if successfully started, false otherwise.
     */
    UFUNCTION(BlueprintCallable)
    bool StreamerStartGenerating(FString StreamerId, FString PlayerToListenTo);
    /**
     * Stops the audio generation process.
     */
    UFUNCTION(BlueprintCallable)
    void StopGenerating();
    /**
     * Checks if the audio generator is currently active.
     * @return True if generating, false otherwise.
     */
    UFUNCTION(BlueprintPure)
    bool IsGenerating() const;

    /**
     * Gets the ID of the currently connected player.
     * @return The connected player's ID.
     */
    UFUNCTION(BlueprintPure)
    FString GetConnectedPlayerId() const;
    /**
     * Gets the ID of the currently connected streamer.
     * @return The connected streamer's ID.
     */
    UFUNCTION(BlueprintPure)
    FString GetConnectedStreamerId() const;

    //~ Begin IPixelStreamingAudioConsumer interface

    /**
     * Consumes raw PCM audio data provided by PixelStreaming.
     * @param AudioData - Pointer to the raw PCM audio data.
     * @param InSampleRate - The sample rate of the audio.
     * @param NChannels - The number of audio channels.
     * @param NFrames - The number of audio frames.
     */
    virtual void ConsumeRawPCM(const int16_t* AudioData, int InSampleRate, size_t NChannels,
                               size_t         NFrames) override;
    /**
     * Callback for when an audio consumer is added, when a new audio sink is available.
     */
    virtual void OnConsumerAdded() override;
    /**
     * Callback for when an audio consumer is removed. Audio generator will stop generating if
     * this is called.
     */
    virtual void OnConsumerRemoved() override;
    //~ End IPixelStreamingAudioConsumer interface

    //~ Begin IOdinAudioControl interface
    virtual bool  GetIsMuted() const override;
    virtual void  SetIsMuted(bool bNewIsMuted) override;
    virtual float GetVolumeMultiplier() const override;
    virtual void  SetVolumeMultiplier(float NewMultiplierValue) override;
    //~ End IOdinAudioControl interface

protected:
    /**
     * Checks if the generator is set to listen to any player.
     * @return True if listening to any player, false otherwise.
     */
    bool WillListenToAnyPlayer() const;

private:
    /** Webrtc Audio sink for PixelStreaming audio */
    IPixelStreamingAudioSink* AudioSink;
    /** ID of the currently connected player */
    FString CurrentPlayerId;
    /** ID of the currently connected streamer */
    FString CurrentStreamerId;
    /** Buffer for storing audio data */
    TArray<float> AudioBuffer;

    /** Flag to indicate if audio generation is active */
    bool bIsGenerating;

    bool  bIsMuted         = false;
    float VolumeMultiplier = 1.0f;
};