There are a number of ways to contribute for people all skills and skill levels.

Translation
-----------

We use the
[Transifex](https://www.transifex.com/ddennedy/shotcut/)
collaboration web site to translate the Shotcut user interface.
Additional languages are already started but have not yet reached the
60% minimum level to be included in a Shotcut release.

Please do not open a pull request to update a translation. Your request
will be rejected unless it is for the English language to add a plural form
or disambiguation comment.

Support Other Users
-------------------

An easy way for non-programmers to help is simply to participate in the
[Shotcut Forum](https://forum.shotcut.org/) by answering user's questions.
Alternatively, consider making a tutorial video on YouTube or similar.

Write Documentation
-------------------

We are now building [documentation in the
forum](https://forum.shotcut.org/c/docs/docs-english) using its wiki-mode where
any trusted user (some forum experience required or manual promotion) can edit
the topic. Please treat this like real documentation you would see in a user
manual. Some day these topics might get compiled into an offline help file or
large document. Initially, we only have English, but it is a sub-category with
the hope that some day a volunteer will offer to translate to another language.

Report a Bug
------------

You can report a bug on the [Shotcut Forum](https://forum.shotcut.org/) using
the Bug category or on [GitHub Issues](https://github.com/mltframework/shotcut/issues/). Bug reports must include the Shotcut version, your operating system,
and the steps. It can helpful to include a screenshot, screencast video, or
project file. Please do not use this to request a feature.

Make a Preset
-------------

After you make a filter preset that you think others would like,
you can share it on the
[Shotcut Forum](https://forum.shotcut.org/) or as a
[GitHub Pull Request](https://github.com/mltframework/shotcut/pulls).
You can locate presets on your system by choosing **Settings > App Data
Directory > Show...** and navigating to the presets folder.

Add a Filter
------------

The engine has many additional filters that are not yet exposed. You can easily
add a Shotcut UI for it.
You can see existing filter UIs and add your own in your Shotcut install folder, look in the folder
share/shotcut/qml/filters
(Shotcut.app/Contents/Resources/shotcut/qml/filters on macOS).

See [How to Make a Plugin](https://www.shotcut.com/notes/make-plugins/)
for more information. You can share your filter UI on the
[Shotcut Forum](https://forum.shotcut.org/) or as a [GitHub Pull
Request](https://github.com/mltframework/shotcut/pulls).

Write Code
----------

The biggest impact you can make is to contribute a code change and submit that
as a [GitHub Pull Request](https://github.com/mltframework/shotcut/pulls).
To make working on Shotcut code and some of its dependencies easier we provide
some SDKs with setup instructions. Of course, you are free to try to work on
the Shotcut code another way, but there is no help provided for that as it gets
very complicated quickly.

Your code format must be consistent with ours and will be checked automatically
using clang-format version 14. You can get that and run something like `ninja clang-format`
and/or `ninja clang-format-check`.

[How To Use the Windows SDK](https://www.shotcut.com/notes/windowsdev/)

All code contributions should assign copyright to Meltytech, LLC; however,
exceptions may be considered for a major contribution.

See our [User Interface Conventions](https://www.shotcut.com/notes/ui-conventions/) for
info about usage of case, alignment, and spacing.
