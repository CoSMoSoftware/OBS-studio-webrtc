package io.cosmosoftware.kite.obs.checks;

import static io.cosmosoftware.kite.entities.Timeouts.DEFAULT_TIMEOUT;
import static io.cosmosoftware.kite.entities.Timeouts.ONE_SECOND_INTERVAL;
import static io.cosmosoftware.kite.entities.Timeouts.TEN_SECOND_INTERVAL;
import static io.cosmosoftware.kite.util.TestUtils.waitAround;
import static io.cosmosoftware.kite.util.WebDriverUtils.poke;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.TestCoordinator;
import io.cosmosoftware.kite.obs.pages.OBSViewerPage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.TestStep;

public class PeerConnectionCheck extends TestStep {

  private final OBSViewerPage obsViewerPage;
  private final TestCoordinator coordinator;

  public PeerConnectionCheck(Runner runner, OBSViewerPage obsViewerPage,
      TestCoordinator coordinator) {
    super(runner);
    this.obsViewerPage = obsViewerPage;
    this.coordinator = coordinator;
    this.setOptional(true);
  }

  @Override
  protected void step() throws KiteTestException {
    waitForStream();
    if (!obsViewerPage.peerConnectionExists()) {
      throw new KiteTestException("Could not verify OBS's peer connection existence",
          Status.FAILED);
    }
  }

  private void waitForStream() throws KiteTestException {
    logger.info("Waiting for stream confirmation");
    for (int wait = 0; wait < DEFAULT_TIMEOUT + ONE_SECOND_INTERVAL * 60;
        wait += ONE_SECOND_INTERVAL) {
      if (coordinator.isObsCrashed()) {
        this.setOptional(false);
        throw new KiteTestException("OBS has crashed while chrome is waiting for stream",
            Status.FAILED);
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
    return "Verify that the peer connection exists for OBS client";
  }
}
