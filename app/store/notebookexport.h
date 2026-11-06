#pragma once

#include "coursebook.h"

class MediaStore;
class QIODevice;

// A course notebook as a ZIP (clickdummy "Exportieren"): notizbuch.md with the course and its
// entries by day, and the photos and voice notes it links to. Plain enough to read anywhere and
// to rework for the Shinki News.
//
//   notizbuch.md                 # Title
//                                6.–7.6.2026 · Siebeldingen · M. D. Nakajima Shihan
//                                ## Samstag, 6. Juni 2026
//                                **15:30** Fußstellung beim Tenkan
//                                ![Foto](bilder/01-foto.jpg)
//                                ⌗ Shiho-nage · Katate-dori · Aikido 5. Kyu
//   bilder/01-foto.jpg
//   audio/02-sprachnotiz.m4a
namespace NotebookExport {

// What the archive would hold, without writing it (the export sheet's preview).
struct Summary {
    QByteArray text;        // notizbuch.md
    int photos = 0;
    int voices = 0;
    int missing = 0;        // media files not on the device
    qint64 bytes = 0;       // the archive's size
};
Summary summary(const CourseBook::Course &course, const QList<CourseBook::Entry> &entries, const MediaStore &media);

// Writes the archive; false when a file could not be written. A missing media file is left
// out (the entry keeps its text) and counted in `missing`.
bool write(QIODevice *out, const CourseBook::Course &course, const QList<CourseBook::Entry> &entries,
           const MediaStore &media, int *missing = nullptr);

// The Markdown alone, with the archive's file names of the media (tests).
QByteArray markdown(const CourseBook::Course &course, const QList<CourseBook::Entry> &entries,
                    const QHash<QString, QString> &files);

// "Lehrgang Siebeldingen 2026-06-06.zip": the notebook's title and first day, safe as a file name.
QString fileName(const CourseBook::Course &course);

}
