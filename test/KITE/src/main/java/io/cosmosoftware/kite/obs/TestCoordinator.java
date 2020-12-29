package io.cosmosoftware.kite.obs;

public class TestCoordinator {

  private String streamName;
  private String publishingToken;
  private String rmptPublishPath;
  private String rtmpPublishStreamName;

  private int streamDuration;

  private boolean streaming;
  private boolean obsCrashed;
  private boolean longetivite;

  public synchronized boolean hasObsCrashed() {
    return obsCrashed;
  }

  public synchronized void setObsCrashed(boolean obsCrashed) {
    this.obsCrashed = obsCrashed;
  }

  public synchronized String getStreamName() {
    return streamName;
  }

  public synchronized void setStreamName(String streamName) {
    this.streamName = streamName;
  }

  public synchronized String getPublishingToken() {
    return publishingToken;
  }

  public synchronized void setPublishingToken(String publishingToken) {
    this.publishingToken = publishingToken;
  }

  public synchronized String getRmptPublishPath() {
    return rmptPublishPath;
  }

  public synchronized void setRmptPublishPath(String rmptPublishPath) {
    this.rmptPublishPath = rmptPublishPath;
  }

  public synchronized String getRtmpPublishStreamName() {
    return rtmpPublishStreamName;
  }

  public synchronized void setRtmpPublishStreamName(String rtmpPublishStreamName) {
    this.rtmpPublishStreamName = rtmpPublishStreamName;
  }

  public synchronized boolean isStreaming() {
    return streaming;
  }

  public synchronized void setStreaming(boolean streaming) {
    this.streaming = streaming;
  }

  public synchronized int getStreamDuration() {
    return streamDuration;
  }

  public synchronized void setStreamDuration(int streamDuration) {
    this.streamDuration = streamDuration;
  }

  public boolean isLongetiviteTest() {
    return longetivite;
  }

  public void setLongetiviteTest(boolean longetivite) {
    this.longetivite = longetivite;
  }
}
