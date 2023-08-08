# xmllint-problem-matcher

This problem matcher lets you show errors from `xmllint` as annotation in GitHub Actions.

## Usage

Add the step to your workflow, before `xmllint` is called.

```yaml
    - uses: korelstar/xmllint-problem-matcher@v1
```
