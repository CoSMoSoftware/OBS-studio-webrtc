package io.cosmosoftware.kite.obs.checks;

import static io.cosmosoftware.kite.entities.Timeouts.DEFAULT_TIMEOUT;
import static io.cosmosoftware.kite.entities.Timeouts.FIVE_SECOND_INTERVAL;
import static io.cosmosoftware.kite.entities.Timeouts.ONE_SECOND_INTERVAL;
import static io.cosmosoftware.kite.entities.Timeouts.SHORT_TIMEOUT;
import static io.cosmosoftware.kite.entities.Timeouts.TEN_SECOND_INTERVAL;
import static io.cosmosoftware.kite.entities.Timeouts.TEN_SECOND_INTERVAL_IN_SECONDS;
import static io.cosmosoftware.kite.util.TestUtils.waitAround;
import static io.cosmosoftware.kite.util.WebDriverUtils.poke;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.TestCoordinator;
import io.cosmosoftware.kite.obs.pages.OBSViewerPage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.StepPhase;
import io.cosmosoftware.kite.steps.TestStep;
import io.cosmosoftware.kite.steps.VideoQualityCheck;

public class StreamingVideoQualityCheck extends TestStep {

  private final Runner runner;
  private final TestCoordinator coordinator;
  private final OBSViewerPage obsViewerPage;
  private final int duration;

  public StreamingVideoQualityCheck(Runner runner, OBSViewerPage obsViewerPage,
      TestCoordinator coordinator, int duration) {
    super(runner);
    this.runner = runner;
    this.coordinator = coordinator;
    this.obsViewerPage = obsViewerPage;
    this.duration = duration;
    this.setOptional(true);
    this.setScreenShotOnFailure(false);
  }

  @Override
  protected void step() throws KiteTestException {
    // this.waitForStream();
    waitAround(FIVE_SECOND_INTERVAL);
    int oneCheckDuration = this.duration / 60;
    if (oneCheckDuration < TEN_SECOND_INTERVAL) {
      oneCheckDuration = TEN_SECOND_INTERVAL; // minimum amount
    }

    if (oneCheckDuration > SHORT_TIMEOUT) {
      oneCheckDuration = SHORT_TIMEOUT;
    }
    for (int wait = FIVE_SECOND_INTERVAL; wait < this.duration - oneCheckDuration;
        wait += oneCheckDuration) {
      VideoQualityCheck check = new VideoQualityCheck(this.runner, this.obsViewerPage, 0,
          "stream-video", true);
      check.setByteComparing(true);
      check.setDuration(FIVE_SECOND_INTERVAL);
      check.setInterval(ONE_SECOND_INTERVAL);
      check.setAllowJerky(true);
      check.setTakeScreenshot(true);
      check.setScreenShotOnFailure(true);
      check.setIgnoreBroken(true);
      if (!this.coordinator.isStreaming()) {
        break;
      }
      try {
        check.processTestStep(StepPhase.DEFAULT, this.report, false);
      } catch (Exception e) {
        logger.info("Video check failed at : " + wait + "ms");
      }
      waitWithInteraction(oneCheckDuration - FIVE_SECOND_INTERVAL);
    }
  }

  private void waitWithInteraction(int duration) {
    for (int wait = 0; wait < duration; wait += ONE_SECOND_INTERVAL) {
      if (wait % FIVE_SECOND_INTERVAL == 0) {
        //this.webDriver.getTitle();
        poke(this.webDriver); // so that browser doesn't time out
      }
      waitAround(ONE_SECOND_INTERVAL);
    }
  }

  private void waitForStream() throws KiteTestException {
    logger.info("Waiting for stream confirmation");
    for (int wait = 0; wait < DEFAULT_TIMEOUT + FIVE_SECOND_INTERVAL * 60;
        wait += ONE_SECOND_INTERVAL) {
      if (coordinator.isObsCrashed()) {
        throw new KiteTestException("OBS has crashed while waiting for stream", Status.FAILED);
      }
      if (!coordinator.isStreaming()) {
        //this.webDriver.getTitle();
        poke(this.webDriver); // so that browser doesn't time out
        waitAround(ONE_SECOND_INTERVAL);
      } else {
        logger.info("Streaming confirmed");
        waitAround(TEN_SECOND_INTERVAL_IN_SECONDS);
        return;
      }
    }
    this.setOptional(false);
    throw new KiteTestException("Could not get stream confirmation after 60s", Status.FAILED);
  }

  private void checkNumberOfVideo() throws KiteTestException {
    if (obsViewerPage.getVideos().size() < obsViewerPage.getNoOfParticipants() + 1) {
      // getNoOfParticipants - 1 for chrome video, 1 streaming video,
      // and 1 hidden video for some reason.
      throw new KiteTestException("Not enough video elements on viewer page", Status.FAILED);
    }
  }

  @Override
  public String stepDescription() {
    return "Verify the streaming video's quality periodically";
  }

}
