"""
Tests for the APU header discovery and multi-root scanning in generate_api_docs.

The APU lives in its own repository (PixelRoot32-APU), so its public headers are
never under the engine's include/ tree — what sits there are re-export shims with
no Doxygen comments. These tests pin the lookup order that brings the real APU
headers back into the generated site.

Run with:
    python -m pytest scripts/test_generate_api_docs.py
"""

import re
import textwrap
from pathlib import Path

import generate_api_docs as gen


APU_MARKER_HEADER = "ApuCore.h"


def write_apu_headers(headers_dir: Path) -> Path:
    """Create a minimal but realistic APU public-header tree."""
    headers_dir.mkdir(parents=True, exist_ok=True)
    (headers_dir / APU_MARKER_HEADER).write_text(
        textwrap.dedent(
            """
            #pragma once

            namespace pixelroot32 { namespace audio {

            /**
             * @class ApuCore
             * @brief NES-style audio processing unit core.
             */
            class ApuCore {
            public:
                void reset();
            };

            } }
            """
        ).strip(),
        encoding="utf-8",
    )
    return headers_dir


def make_apu_repo(root: Path) -> Path:
    """Create a checkout-shaped APU repo: <root>/include/pixelroot32/apu/*.h"""
    write_apu_headers(root / "include" / "pixelroot32" / "apu")
    return root


class TestFindApuIncludeDir:
    def test_env_override_wins_over_every_other_candidate(self, tmp_path):
        engine_root = tmp_path / "engine"
        override_repo = make_apu_repo(tmp_path / "explicit-apu")
        # A PlatformIO-resolved copy also exists; the override must still win.
        make_apu_repo(engine_root / ".pio" / "libdeps" / "native_test" / "PixelRoot32-APU")

        found = gen.find_apu_include_dir(
            engine_root, env={"PIXELROOT32_APU_ROOT": str(override_repo)}
        )

        assert found == (override_repo / "include" / "pixelroot32" / "apu").resolve()

    def test_env_override_accepts_the_headers_dir_directly(self, tmp_path):
        engine_root = tmp_path / "engine"
        headers = write_apu_headers(tmp_path / "apu-headers" / "pixelroot32" / "apu")

        found = gen.find_apu_include_dir(
            engine_root, env={"PIXELROOT32_APU_ROOT": str(headers)}
        )

        assert found == headers.resolve()

    def test_falls_back_to_platformio_libdeps(self, tmp_path):
        engine_root = tmp_path / "engine"
        libdep = make_apu_repo(
            engine_root / ".pio" / "libdeps" / "native_test" / "PixelRoot32-APU"
        )

        found = gen.find_apu_include_dir(engine_root, env={})

        assert found == (libdep / "include" / "pixelroot32" / "apu").resolve()

    def test_libdeps_beats_sibling_checkout(self, tmp_path):
        # The resolved dependency reflects the version library.json pins, so it is
        # the more trustworthy source when both are present.
        workspace = tmp_path / "workspace"
        engine_root = workspace / "PixelRoot32-Game-Engine"
        make_apu_repo(workspace / "PixelRoot32-APU")
        libdep = make_apu_repo(
            engine_root / ".pio" / "libdeps" / "native_test" / "PixelRoot32-APU"
        )

        found = gen.find_apu_include_dir(engine_root, env={})

        assert found == (libdep / "include" / "pixelroot32" / "apu").resolve()

    def test_falls_back_to_sibling_checkout_walking_up_parents(self, tmp_path):
        # Mirrors the real nested layout: Game-Samples/lib/PixelRoot32-Game-Engine
        workspace = tmp_path / "workspace"
        engine_root = workspace / "PixelRoot32-Game-Samples" / "lib" / "PixelRoot32-Game-Engine"
        engine_root.mkdir(parents=True)
        sibling = make_apu_repo(workspace / "PixelRoot32-APU")

        found = gen.find_apu_include_dir(engine_root, env={})

        assert found == (sibling / "include" / "pixelroot32" / "apu").resolve()

    def test_returns_none_when_the_apu_is_nowhere(self, tmp_path):
        engine_root = tmp_path / "engine"
        engine_root.mkdir()

        assert gen.find_apu_include_dir(engine_root, env={}) is None

    def test_ignores_a_candidate_without_the_marker_header(self, tmp_path):
        engine_root = tmp_path / "engine"
        empty = tmp_path / "not-really-the-apu"
        (empty / "include" / "pixelroot32" / "apu").mkdir(parents=True)

        assert (
            gen.find_apu_include_dir(engine_root, env={"PIXELROOT32_APU_ROOT": str(empty)})
            is None
        )


class TestScanIncludeDirectoryModuleOverride:
    def test_override_groups_every_class_under_one_module(self, tmp_path):
        headers = write_apu_headers(tmp_path / "include" / "pixelroot32" / "apu")

        modules = gen.scan_include_directory(str(headers), module_override="apu")

        assert list(modules) == ["apu"]
        assert [cls.name for cls in modules["apu"]] == ["ApuCore"]
        assert modules["apu"][0].namespace == "apu"

    def test_without_override_the_module_still_comes_from_the_directory(self, tmp_path):
        include_dir = tmp_path / "include"
        write_apu_headers(include_dir / "audio")

        modules = gen.scan_include_directory(str(include_dir))

        assert list(modules) == ["audio"]


class TestCodeBlocks:
    """
    @code/@endcode must survive as fenced Markdown.

    VitePress compiles every page as a Vue SFC, so a C++ snippet left as a bare
    paragraph makes `static_cast<StateId>` parse as an unclosed HTML tag and the
    whole docs build fails.
    """

    def parse(self, body: str) -> gen.DocComment:
        return gen.parse_doc_comment(body)

    def test_code_block_becomes_a_cpp_fence(self):
        doc = self.parse(
            textwrap.dedent(
                """
                /**
                 * @class StateMachine
                 * @brief Finite state machine.
                 *
                 * The convention is a static table:
                 *
                 * @code
                 * static const StateMachine::State kStates[] = {
                 *     { onEnterIdle, static_cast<StateId>(PlayerState::IDLE) },
                 * };
                 * @endcode
                 *
                 * Transitions are synchronous.
                 */
                """
            ).strip()
        )

        assert "```cpp" in doc.description
        assert "static_cast<StateId>(PlayerState::IDLE)" in doc.description
        # The prose on both sides of the fence must survive too.
        assert "The convention is a static table:" in doc.description
        assert "Transitions are synchronous." in doc.description

    def test_every_angle_bracket_stays_inside_a_fence(self):
        doc = self.parse(
            textwrap.dedent(
                """
                /**
                 * @class Widget
                 * @brief Does things.
                 *
                 * @code
                 * auto v = std::vector<int>{1, 2};
                 * @endcode
                 */
                """
            ).strip()
        )

        for line in doc.description.splitlines():
            if "<" in line:
                assert _is_inside_fence(doc.description, line), (
                    f"unfenced angle bracket would break the Vue compiler: {line!r}"
                )

    def test_brief_stops_before_the_code_block(self):
        doc = self.parse(
            textwrap.dedent(
                """
                /**
                 * @class Widget
                 * @brief Does things.
                 * @code
                 * int x = 1;
                 * @endcode
                 */
                """
            ).strip()
        )

        assert doc.brief == "Does things."
        assert "int x = 1;" not in doc.brief

    def test_multiple_code_blocks_are_all_preserved(self):
        doc = self.parse(
            textwrap.dedent(
                """
                /**
                 * @class Widget
                 * @brief Does things.
                 *
                 * @code
                 * first();
                 * @endcode
                 *
                 * @code
                 * second();
                 * @endcode
                 */
                """
            ).strip()
        )

        assert "first();" in doc.description
        assert "second();" in doc.description
        assert doc.description.count("```cpp") == 2

    def test_a_comment_without_code_is_unchanged(self):
        doc = self.parse(
            textwrap.dedent(
                """
                /**
                 * @class Widget
                 * @brief Does things.
                 *
                 * Extra prose here.
                 */
                """
            ).strip()
        )

        assert doc.brief == "Does things."
        assert "```" not in doc.description
        assert "Extra prose here." in doc.description


class TestEscapeHtmlInProse:
    """
    Template names written in plain prose (`RoomGraph<N>`) are the other way a
    generated page breaks the Vue compiler. Escape them, but never touch code.
    """

    def test_escapes_a_template_name_in_prose(self):
        assert (
            gen.escape_html_in_prose("Abstract base for RoomGraph<N> used by Scene.")
            == "Abstract base for RoomGraph&lt;N> used by Scene."
        )

    def test_leaves_inline_code_spans_alone(self):
        line = "Prefer `std::vector<int>` over raw arrays."
        assert gen.escape_html_in_prose(line) == line

    def test_leaves_fenced_blocks_alone(self):
        text = "Prose with Grid<N>.\n\n```cpp\nstd::vector<int> v;\n```\n\nMore Grid<N>."
        result = gen.escape_html_in_prose(text)

        assert "std::vector<int> v;" in result
        assert result.count("Grid&lt;N>") == 2

    def test_leaves_comparison_operators_alone(self):
        assert gen.escape_html_in_prose("valid when a < b and c > d") == "valid when a < b and c > d"

    def test_is_a_no_op_without_angle_brackets(self):
        assert gen.escape_html_in_prose("Plain sentence.") == "Plain sentence."

    def test_rendered_class_page_has_no_unfenced_tag_like_text(self):
        cls = gen.ClassDoc(
            name="RoomGraphBase",
            type="class",
            namespace="gameplay",
            source_file="RoomGraph.h",
            doc=gen.DocComment(
                brief="Abstract base class for RoomGraph<N> used by Scene.",
                description="RoomGraph<N> publicly inherits from this base.",
                notes=["Lives in the RoomGraph<N> template."],
                warnings=["Do not use Grid<N> directly."],
            ),
            methods=[
                gen.Method(
                    name="visit",
                    signature="void visit(RoomGraphBase* g)",
                    doc=gen.DocComment(
                        brief="Visits a RoomGraph<N>.",
                        params={"g": "Pointer to a RoomGraph<N> instance."},
                        return_doc="A Handle<N> for the visit.",
                    ),
                )
            ],
            properties=[gen.Property(name="graph", type="RoomGraphBase*", doc="The Graph<N> in use.")],
        )

        markdown = gen.generate_class_markdown(cls, {})

        for line in markdown.splitlines():
            if line.startswith("<Badge"):
                continue
            assert not re.search(r"<[A-Za-z_]", line), f"tag-like text survived: {line!r}"


def _is_inside_fence(text: str, target_line: str) -> bool:
    inside = False
    for line in text.splitlines():
        if line.strip().startswith("```"):
            inside = not inside
            continue
        if line == target_line:
            return inside
    return False


class TestReadApuVersion:
    def test_reads_the_version_from_library_json(self, tmp_path):
        repo = make_apu_repo(tmp_path / "apu")
        (repo / "library.json").write_text('{"version": "2.0.0"}', encoding="utf-8")

        headers = repo / "include" / "pixelroot32" / "apu"
        assert gen.read_apu_version(headers) == "2.0.0"

    def test_returns_none_when_library_json_is_missing(self, tmp_path):
        repo = make_apu_repo(tmp_path / "apu")

        headers = repo / "include" / "pixelroot32" / "apu"
        assert gen.read_apu_version(headers) is None
