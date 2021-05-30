#!/usr/bin/env python3

import gnupg
import hashlib

def gen_release_files(archive):
    gpg = gnupg.GPG()

    stream = open(archive, 'rb')
    release_file = stream.read()

    gpg.sign_file(stream, detach=True, output='{0}.sig'.format(archive))
    sha256hash = hashlib.sha256(release_file)
    sha256digest = sha256hash.hexdigest()

    sha256file = open('{0}.sha256sum'.format(archive), 'w')
    print(sha256digest, file = sha256file)
    sha256file.close()
    stream.close()
