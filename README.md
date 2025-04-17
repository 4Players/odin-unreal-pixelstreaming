# 4Players ODIN Unreal Pixel Streaming
Plugin to support 4Players ODIN technology when using Unreal's PixelStreaming. Please take a look at the documentation for the Odin Voice Plugin for Unreal [https://docs.4players.io/voice/unreal](https://docs.4players.io/voice/unreal) for more information.

## Requirements

- ODIN Voice Chat Plugin Version 1.9.0 or higher: https://github.com/4Players/odin-sdk-unreal/releases/latest
- Unreal Pixel Streaming
- Please be aware, that you'll need to provide your PixelStreaming Web Client using https in production, otherwise most browsers will deny Microphone Access

## Usage

The `OdinPixelStreamingAudioGenerator` needs to replace the `AudioCapture` object when calling `Construct Local Media` Function. The Odin Pixel Streaming Audio Generator object will connect to the microphone input stream from the Web Client that's connected to the Streamer Machine. You'll need to call `StartGenerating` on the Audio Generator object with the Player Id of the connected Web Client (you can get the player id's from Unreal's Pixel Streaming Subsystem). You'll also need to call `StopGenerating` in case the Web Client disconnects from the streamer.
