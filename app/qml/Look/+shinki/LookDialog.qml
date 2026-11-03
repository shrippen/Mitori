import Shinki

ShinkiDialog {
    enum Span {
        Fit,
        Small,
        Medium,
        Large
    }
    property int span: LookDialog.Span.Fit

    size: span === LookDialog.Span.Small ? ShinkiDialog.Size.Small
        : span === LookDialog.Span.Medium ? ShinkiDialog.Size.Medium
        : span === LookDialog.Span.Large ? ShinkiDialog.Size.Large : ShinkiDialog.Size.Fit
}
