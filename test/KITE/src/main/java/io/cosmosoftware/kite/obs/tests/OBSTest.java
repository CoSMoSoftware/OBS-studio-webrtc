package io.cosmosoftware.kite.obs.tests;

import static io.cosmosoftware.kite.entities.Timeouts.SHORT_TIMEOUT;

import io.cosmosoftware.kite.obs.TestCoordinator;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.obs.steps.ChangeCodecStep;
import io.cosmosoftware.kite.obs.steps.PrepareStreamStep;
import io.cosmosoftware.kite.obs.steps.QuitApplicationStep;
import java.util.ArrayList;
import java.util.List;
import javax.json.JsonArray;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.remote.RemoteWebDriver;
import org.webrtc.kite.tests.KiteBaseTest;
import org.webrtc.kite.tests.TestRunner;

public class OBSTest extends KiteBaseTest {

  protected final TestCoordinator coordinator = new TestCoordinator();
  protected int streamDuration = SHORT_TIMEOUT;
  protected List<String> codecs = new ArrayList<>();
  protected boolean stereo = false;


  @Override
  protected void payloadHandling() {
    super.payloadHandling();
    this.stereo = payload.getBoolean("stereo", stereo);
    coordinator.setStreamDuration(this.meetingDuration);

    codecs.add("H264"); // by default, can be changed
    if (payload.get("codec") != null) {
      JsonArray codecArray = payload.getJsonArray("codec");
      if (!codecArray.isEmpty()) {
        codecs.clear();
        for (int i = 0; i < codecArray.size(); i++) {
          this.codecs.add(codecArray.getString(i).toUpperCase());
        }
      }
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
//        for (String bitrate : videoBitrate) {
//          testRunner.addStep(new ChangeBitrateStep(testRunner, bitrate));
//          testRunner.addStep(new StartStreamingStep(testRunner, streamDuration, coordinator));
//          if (longevity) {
//            testRunner.addStep(new MonitoringCheck(testRunner, coordinator, streamDuration));
//          } else {
//            testRunner.addStep(new GetStatsStep(testRunner, null, mainPage));
//          }
//          testRunner.addStep(new StopStreamingStep(testRunner, coordinator));
//        }
      }
      testRunner.addStep(new QuitApplicationStep(testRunner, coordinator));
    } else {
//      OBSViewerPage obsViewerPage = new OBSViewerPage(testRunner, this.tupleSize,
//          this.peerConnectionPrefix);
//      // assuming the id for the browser is from 1 to N
//      testRunner.addStep(new JoinRoomStep(testRunner, obsViewerPage, this.roomManager.getRoomUrl(),
//          (credentials.size() >= id) ? credentials.get(id - 1) : null, id == 1, this.coordinator));
//      testRunner.addStep(new ResizeWindowStep(testRunner, 1000, 1000));
//      if (!getinfo) {
//        testRunner.addStep(new GetStreamInfoStep(testRunner, coordinator, obsViewerPage));
//        this.getinfo = true;
//      }
//      for (String codec : this.codecs) {
//        for (String bitrate : videoBitrate) {
//          PeerConnectionCheck peerConnectionCheck = new PeerConnectionCheck(testRunner,
//              obsViewerPage, coordinator);
//          testRunner.addStep(peerConnectionCheck);
//          if (longevity) {
//            testRunner
//                .addStep(new StreamingVideoQualityCheck(testRunner, obsViewerPage, coordinator,
//                    this.streamDuration), peerConnectionCheck);
//          } else {
//            testRunner
//                .addStep(new StreamingVideoDisplayCheck(testRunner, obsViewerPage, coordinator),
//                    peerConnectionCheck);
//            testRunner.addStep(new GetStatsStep(testRunner, this.getStatsConfig, obsViewerPage),
//                peerConnectionCheck);
//            if (!videoBitrate.isEmpty()) {
//              testRunner
//                  .addStep(new VideoBitrateCheck(testRunner, bitrate, obsViewerPage),
//                      peerConnectionCheck);
//            }
//            if (stereo) {
//              testRunner
//                  .addStep(new StereoAudioCheck(testRunner, obsViewerPage), peerConnectionCheck);
//            }
//            testRunner.addStep(new IdlingStep(testRunner, this.streamDuration, coordinator));
//          }
//        }
//      }
//      testRunner.addStep(new WebRTCInternalsStep(testRunner));
    }
  }

  private boolean isApp(WebDriver webDriver) {
    return ((RemoteWebDriver) webDriver).getCapabilities().getBrowserName().equals("app");
  }
}
