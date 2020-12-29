package io.cosmosoftware.kite.obs.pages.settings;

import static io.cosmosoftware.kite.entities.Timeouts.ONE_SECOND_INTERVAL;
import static io.cosmosoftware.kite.entities.Timeouts.THREE_SECOND_INTERVAL;
import static io.cosmosoftware.kite.util.TestUtils.waitAround;

import io.cosmosoftware.kite.exception.KiteInteractionException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.Page;
import io.cosmosoftware.kite.obs.pages.elements.PageElements;
import io.cosmosoftware.kite.obs.pages.elements.SettingPageElements;
import org.openqa.selenium.remote.RemoteWebDriver;

public abstract class SettingPage extends Page {

  private final SettingPageElements settingPageElements = new SettingPageElements();

  protected SettingPage(Runner runner) {
    super(runner);
    this.os = ((RemoteWebDriver) runner.getWebDriver()).getCapabilities().getPlatform().toString();
    this.elements = this.settingPageElements.populateElement(this.os);
    this.addExtraElements(this.os);
  }


  public void openSettingOption() throws KiteInteractionException {
    click(getElement(this.getSettingOptionName()));
    waitAround(ONE_SECOND_INTERVAL);
  }

  public void applyChange() throws KiteInteractionException {
    click(getElement(PageElements.APPLY_BTN));
    waitAround(ONE_SECOND_INTERVAL);
    click(getElement(PageElements.OK_BTN));
    waitAround(THREE_SECOND_INTERVAL);
  }

  public void discardChange() throws KiteInteractionException {
    click(getElement(PageElements.CANCEL_BTN));
  }

  protected abstract String getSettingOptionName();

  protected void addExtraElements(String os) {
    if (os.equals("WINDOWS")) {
      addWindowsElements();
    } else if (os.equals("MAC")) {
      addMacElements();
    } else {
      addLinuxElements();
    }
  }

  abstract protected void addWindowsElements();

  abstract protected void addMacElements();

  abstract protected void addLinuxElements();

}
