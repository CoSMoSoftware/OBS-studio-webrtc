package io.cosmosoftware.kite.obs.tests;

import io.cosmosoftware.kite.obs.checks.StreamingVideoDisplayCheck;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.obs.pages.ViewerPage;
import io.cosmosoftware.kite.obs.steps.ChangeCodecStep;
import io.cosmosoftware.kite.obs.steps.PrepareStreamStep;
import io.cosmosoftware.kite.obs.steps.QuitApplicationStep;
import io.cosmosoftware.kite.obs.steps.StartStreamingStep;
import io.cosmosoftware.kite.obs.steps.StopStreamingStep;
import io.cosmosoftware.kite.steps.OpenURLStep;
import io.cosmosoftware.kite.steps.StepPhase;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.remote.RemoteWebDriver;
import org.webrtc.kite.stats.GetStatsStep;
import org.webrtc.kite.tests.TestRunner;

public class OBSInteropTest extends OBSTest {

  @Override
  protected void payloadHandling() {
    super.payloadHandling();
    if (payload.get("publishingInfo") != null) {
      this.coordinator.setStreamName(payload.getJsonObject("publishingInfo").getString("streamName"));
      this.coordinator.setPublishingToken(payload.getJsonObject("publishingInfo").getString("publishingToken"));
    }
  }


  @Override
  protected void populateTestSteps(TestRunner testRunner) {
    int id = testRunner.getId();
    if (isApp(testRunner.getWebDriver())) {
      // assuming the id for the app is always 0
      MainPage mainPage = new MainPage(testRunner);
      testRunner.addStep(new PrepareStreamStep(testRunner, coordinator));
      for (String codec : this.codecs) {
        testRunner.addStep(new ChangeCodecStep(testRunner, codec));
        testRunner.addStep(new StartStreamingStep(testRunner, streamDuration, coordinator));
        testRunner.addStep(new StopStreamingStep(testRunner, coordinator));
      }
      testRunner.addStep(new QuitApplicationStep(testRunner, coordinator));
    } else {
      for (String codec : this.codecs) {
        testRunner.addStep(new OpenURLStep(testRunner, this.url));
        testRunner.addStep(
            new StreamingVideoDisplayCheck(testRunner, new ViewerPage(testRunner), coordinator));
        GetStatsStep getStatsStep = new GetStatsStep(testRunner, this.getStatsConfig);
        getStatsStep.setNeverSkip(true);
        getStatsStep.setScreenShotOnFailure(false);
        testRunner.addStep(getStatsStep, StepPhase.ALL);
      }
    }
  }

  private boolean isApp(WebDriver webDriver) {
    return ((RemoteWebDriver) webDriver).getCapabilities().getBrowserName().equals("app");
  }
}
