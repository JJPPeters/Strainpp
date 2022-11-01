//
// Created by jon on 01/11/2022.
//

#include "versiondialog.h"

extern "C"
{
extern const char* GIT_TAG;
extern const char* GIT_REV;
extern const char* GIT_BRANCH;
}

const char* libfive_git_version(void)
{
    return GIT_TAG;
}

const char* libfive_git_revision(void)
{
    return GIT_REV;
}

const char* libfive_git_branch(void)
{
    return GIT_BRANCH;
}

std::string make_version_string() {
    std::string out = "Strain++ - ";
    std::string ver = libfive_git_version();
    std::string hsh = libfive_git_revision();
    std::string brn = libfive_git_branch();
    if (!ver.empty())
        return out + ver + " - " + hsh;
    else
        return out + brn + " - " + hsh;
}

void show_version_dialog() {
    QMessageBox msgBox;
    msgBox.setText(QString::fromStdString(make_version_string()));
    msgBox.exec();
}