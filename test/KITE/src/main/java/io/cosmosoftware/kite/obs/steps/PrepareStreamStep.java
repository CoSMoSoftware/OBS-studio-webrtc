package io.cosmosoftware.kite.obs.steps;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.TestCoordinator;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.obs.pages.settings.StreamSettingPage;
import io.cosmosoftware.kite.steps.TestStep;

public class PrepareStreamStep extends TestStep {

  private final MainPage mainPage;
  private final StreamSettingPage streamSettingPage;
  private final TestCoordinator coordinator;
  private final String MAC_SERVICE_FILE = "~/Library/Application Support/obs-webrtc/basic/profiles/Untitled/service.json";
  private final String WNDOWS_SERVICE_FILE = "C:\\Users\\Nam\\AppData\\Roaming\\obs-webrtc\\basic\\profiles\\Untitled\\service.json";

  public PrepareStreamStep(Runner runner, TestCoordinator coordinator) {
    super(runner);
    this.mainPage = new MainPage(runner);
    this.streamSettingPage = new StreamSettingPage(runner);
    this.coordinator = coordinator;
  }

  @Override
  protected void step() throws KiteTestException {
    mainPage.openSettings();
    streamSettingPage.openSettingOption();
    try {
      if (coordinator.getRmptPublishPath()!= null ){
        streamSettingPage.inputRtmpStreamKey(this.coordinator.getRtmpPublishStreamName());
        streamSettingPage.inputRtmpServerName(this.coordinator.getRmptPublishPath());
      } else {
        streamSettingPage.inputStreamName(this.coordinator.getStreamName());
        streamSettingPage.inputPublishingToken(this.coordinator.getPublishingToken());
      }
    } catch (Exception e) {
      throw e;
    } finally {
      streamSettingPage.applyChange();
    }
  }

  @Override
  public String stepDescription() {
    return "Input parameters for streaming in Settings";
  }

}
