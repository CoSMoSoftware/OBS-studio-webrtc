package io.cosmosoftware.kite.obs.steps;

import static io.cosmosoftware.kite.entities.Timeouts.ONE_SECOND_INTERVAL;
import static io.cosmosoftware.kite.util.TestUtils.waitAround;
import static io.cosmosoftware.kite.util.WebDriverUtils.poke;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.TestCoordinator;
import io.cosmosoftware.kite.steps.TestStep;

public class IdlingStep extends TestStep {

  private final int streamDuration;
  private final TestCoordinator coordinator;

  public IdlingStep(Runner runner, int streamDuration, TestCoordinator coordinator) {
    super(runner);
    this.coordinator = coordinator;
    this.streamDuration = streamDuration;
  }

  @Override
  protected void step() throws KiteTestException {
    this.coordinator.setStreaming(true);
    for (int wait = 0; wait < streamDuration; wait += ONE_SECOND_INTERVAL) {
      waitAround(ONE_SECOND_INTERVAL);
      //this.webDriver.getTitle();
      poke(this.webDriver); // to make sure the session doesn't time out if wait too long
      if (!this.coordinator.isStreaming()) {
        return;
      }
    }
  }

  @Override
  public String stepDescription() {
    return "Idling until the stream ends";
  }
}
