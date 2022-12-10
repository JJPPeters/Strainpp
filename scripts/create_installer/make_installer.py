import shutil, os, re, subprocess

mingw_path = r'D:\Programming\packages\msys64\mingw64'

build_path = r'D:\Work\strainpp-dev\Strainpp-git\cmake-build-release'

inno_exe = r'C:\Program Files (x86)\Inno Setup 6\ISCC.exe'

this_path = os.path.dirname(os.path.abspath(__file__))

# WARNING: this folder will get deleted, so don't set it to anything important
temp_path = os.path.join(this_path, 'temp')

def copy_dist_files():

    dist_path = os.path.join(temp_path, 'dist_files') # this is the path of this file

    os.makedirs(dist_path, exist_ok=True)

    bin_path = os.path.join(mingw_path, "bin")
    bin_deps = ['zlib1',
                'Qt5Widgets',
                'Qt5Svg',
                'Qt5PrintSupport',
                'Qt5Gui',
                'Qt5Core',
                'libwinpthread-1',
                'libturbojpeg',
                'libtiffxx-5',
                'libtiff-5',
                'libstdc++-6',
                'libpng16-16',
                'libpcre16-0',
                'libpcre2-16-0',
                'liblzma-5',
                'libjpeg-8',
                'libintl-8',
                'libicuuc67',
                'libicuin67',
                'libicudt67',
                'libiconv-2',
                'libharfbuzz-0',
                'libgomp-1',
                'libgobject-2.0-0',
                'libglib-2.0-0',
                'libgcc_s_seh-1',
                'libfreetype-6',
                'libfftw3-3',
                'libffi-7',
                'libbz2-1',
                'libgraphite2',
                'libpcre-1',
                'libdouble-conversion',
                'libzstd',
                'libbrotlidec',
                'libbrotlicommon']

    formats_path = os.path.join(mingw_path, r"share\qt5\plugins\imageformats")
    formats_deps = ['qtiff']

    styles_path = os.path.join(mingw_path, r"share\qt5\plugins\styles")
    styles_deps = ['qwindowsvistastyle']

    platform_path = os.path.join(mingw_path, r"share\qt5\plugins\platforms")
    platform_deps = ['qminimal',
                    'qwindows']

    other_deps = [r'D:\Programming\libraries\qcustomplot\2.0.1\lib\qcustomplot2.dll',
                  os.path.join(build_path, r'strainpp.exe')]

    for dep in bin_deps:
        try:
            shutil.copy(os.path.join(bin_path, dep+'.dll'), dist_path)
        except IOError as e:
            print('Error: ' + os.path.join(bin_path, dep+'.dll') + ', ' + e.strerror)

    for dep in formats_deps:
        pth = os.path.join(dist_path, 'imageformats')
        os.makedirs(pth, exist_ok=True)

        try:
            shutil.copy(os.path.join(formats_path, dep+'.dll'), pth)
        except IOError as e:
            print('Error: ' + dep + '.dll, ' + e.strerror)

    for dep in styles_deps:
        pth = os.path.join(dist_path, 'styles')
        os.makedirs(pth, exist_ok=True)

        try:
            shutil.copy(os.path.join(styles_path, dep+'.dll'), pth)
        except IOError as e:
            print('Error: ' + dep + '.dll, ' + e.strerror)

    for dep in platform_deps:
        pth = os.path.join(dist_path, 'platforms')
        os.makedirs(pth, exist_ok=True)

        try:
            shutil.copy(os.path.join(platform_path, dep+'.dll'), pth)
        except IOError as e:
            print('Error: ' + dep + '.dll, ' + e.strerror)

    for dep in other_deps:
        try:
            shutil.copy(dep, dist_path)
        except IOError as e:
            print('Error: ' + dep + e.strerror)


def get_versions():
    version_file = os.path.join(build_path, r'src\version.cpp')

    version = ""
    branch = ""
    revision = ""

    with open(version_file, 'r') as file:
        for line in file:
            mtch = re.findall(r'["](.+)["]', line)

            if not mtch:
                continue

            if "GIT_REV" in line:
                revision = mtch[0]
            elif "GIT_TAG" in line:
                version = mtch[0]
            elif "GIT_BRANCH" in line:
                branch = mtch[0]

    return version, branch, revision


def make_version_string():
    version, branch, revision = get_versions()

    if version:
        return version
    else:
        return branch + "." + revision


def make_installer_script():
    in_script_name = "inno_installer_script.iss"
    in_script_path = os.path.join(this_path, in_script_name)

    out_script_name = "strainpp.iss"
    out_script_path = os.path.join(temp_path, out_script_name)

    version_string = make_version_string()

    with open(in_script_path, 'r') as in_file:
        file_text = in_file.read()

        file_text = file_text.replace(";version_string;", version_string)

        with open(out_script_path, 'w') as out_file:
            out_file.write(file_text)

    return out_script_path

def make_installer():
    if os.path.exists(temp_path):
        shutil.rmtree(temp_path)

    # copy dependencies
    copy_dist_files()

    # copy installer files to temp folder
    src_path = os.path.join(this_path, "installer_files")
    dst_path = os.path.join(temp_path, "installer_files")

    shutil.copytree(src_path, dst_path) 

    # make script with correct version
    script_path = make_installer_script()
    
    # run the script
    process = subprocess.run([inno_exe, script_path], 
                             stdout=subprocess.PIPE, 
                             universal_newlines=True)

if __name__ == "__main__":
    make_installer()