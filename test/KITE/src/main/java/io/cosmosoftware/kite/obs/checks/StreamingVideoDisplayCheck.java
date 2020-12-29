package io.cosmosoftware.kite.obs.checks;

import static io.cosmosoftware.kite.entities.Timeouts.DEFAULT_TIMEOUT;
import static io.cosmosoftware.kite.entities.Timeouts.ONE_SECOND_INTERVAL;
import static io.cosmosoftware.kite.entities.Timeouts.TEN_SECOND_INTERVAL;
import static io.cosmosoftware.kite.util.TestUtils.waitAround;
import static io.cosmosoftware.kite.util.WebDriverUtils.poke;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.TestCoordinator;
import io.cosmosoftware.kite.obs.pages.ViewerPage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.VideoQualityCheck;

public class StreamingVideoDisplayCheck extends VideoQualityCheck {

  private final TestCoordinator coordinator;
  private final ViewerPage viewerPage;

  public StreamingVideoDisplayCheck(Runner runner, ViewerPage viewerPage,
      TestCoordinator coordinator) {
    super(runner, viewerPage, 0, "Streaming video", true);
    this.coordinator = coordinator;
    this.viewerPage = viewerPage;
  }


  @Override
  protected void step() throws KiteTestException {
     waitForStream();
    super.step();
  }

  private void waitForStream() throws KiteTestException {
    logger.info("Waiting for stream confirmation");
    for (int wait = 0; wait < DEFAULT_TIMEOUT + this.coordinator.getStreamDuration();
        wait += ONE_SECOND_INTERVAL) {
      if (coordinator.isObsCrashed()) {
        this.setOptional(false);
        throw new KiteTestException("OBS has crashed while waiting for stream", Status.FAILED);
      }
      if (!coordinator.isStreaming()) {
        //this.webDriver.getTitle();
        poke(this.webDriver); // so that browser doesn't time out
        waitAround(ONE_SECOND_INTERVAL);
      } else {
        logger.info("Streaming confirmed");
        waitAround(TEN_SECOND_INTERVAL);
        return;
      }
    }
    this.setOptional(false);
    throw new KiteTestException("Could not get stream confirmation after 60s", Status.FAILED);
  }

  @Override
  public String stepDescription() {
    return "Verify the streaming video's display";
  }
}
