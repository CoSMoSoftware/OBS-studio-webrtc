package io.cosmosoftware.kite.obs.pages;

import static io.cosmosoftware.kite.entities.Timeouts.TEN_SECOND_INTERVAL_IN_SECONDS;
import static io.cosmosoftware.kite.util.TestUtils.executeJsScript;

import io.cosmosoftware.kite.exception.KiteInteractionException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.pages.BasePage;
import java.util.List;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.support.FindBy;

public class ViewerPage extends BasePage {

  @FindBy(tagName="video")
  private List<WebElement> videos;

  @FindBy(id="container")
  private WebElement container;  
  
  public ViewerPage(Runner runner) {
    super(runner);
  }
  
  public void open(String url) throws KiteInteractionException {
    loadPage(url, 20);
  }

  public void playVideo() throws KiteInteractionException {
    //first interact with the page (see https://developers.google.com/web/updates/2017/09/autoplay-policy-changes)
    waitUntilVisibilityOf(container, TEN_SECOND_INTERVAL_IN_SECONDS);
    click(container);
    //then play the video
    executeJsScript(webDriver,"document.getElementsByTagName('video')[0].play();");
  }
  
}
