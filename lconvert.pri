# Copyright © 2019-2022 Pedro López-Cabanillas <plcl@users.sf.net>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

qtPrepareTool(LCONVERT, lconvert)
isEmpty(LCONVERT_LANGS): LCONVERT_LANGS = es en
isEmpty(LCONVERT_PATTERNS): LCONVERT_PATTERNS = qtbase qtmultimedia qtscript qtxmlpatterns
LCONVERT_OUTPUTS =
for(lang, LCONVERT_LANGS) {
    lang_files =
    for(pat, LCONVERT_PATTERNS) {
        lang_files += $$files($$[QT_INSTALL_TRANSLATIONS]/$${pat}_$${lang}.qm)
    }
    outfile = $$OUT_PWD/qt_$${lang}.qm
    !isEmpty(lang_files): system("$$LCONVERT -i $$join(lang_files, ' ') -o $$outfile"): LCONVERT_OUTPUTS += $$outfile
}
qm_res.files = $$LCONVERT_OUTPUTS
qm_res.base = $$OUT_PWD
qm_res.prefix = "/"
RESOURCES += qm_res
