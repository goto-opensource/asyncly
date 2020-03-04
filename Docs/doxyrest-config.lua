-- Specify input and output paths:

OUTPUT_FILE = os.getenv("TMP_RST_DIR") .. "/index.rst"
INPUT_FILE = os.getenv("DOXYGEN_XML_DIR") .. "/index.xml"
FRAME_FILE = "index.rst.in"
FRAME_DIR_LIST = { os.getenv("DOXYREST_FRAME_DIR") .. "/cfamily", os.getenv("DOXYREST_FRAME_DIR") .. "/common" }

-- Usually, Doxygen-based documentation has a main page (created with
-- the \mainpage directive). If that's the case, force-include
-- the contents of 'page_index.rst' into 'index.rst':

INTRO_FILE = "page_index.rst"

-- If your documentation uses \verbatim directives for code snippets
-- you can convert those to reStructuredText C++ code-blocks:

VERBATIM_TO_CODE_BLOCK = "cpp"

-- Asterisks, pipes and trailing underscores have special meaning in
-- reStructuredText. If they appear in Doxy-comments anywhere except
-- for code-blocks, they must be escaped:

ESCAPE_ASTERISKS = true
ESCAPE_PIPES = true
ESCAPE_TRAILING_UNDERSCORES = true