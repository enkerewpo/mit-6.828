#!/bin/bash

set -e

XV6_REPO="/home/wheatfox/tryredox/MIT/xv6-labs-2024"
MIT_LABS="/home/wheatfox/tryredox/MIT/mit-6.828"
PATCHES_DIR="$MIT_LABS/patches"

declare -A LAB_BRANCH_MAP=(
    ["lab1"]="util"
    ["lab2"]="syscall" 
    ["lab3"]="pgtbl"
    ["lab4"]="traps"
    ["lab5"]="cow"
    ["lab6"]="net"
    ["lab7"]="lock"
    ["lab8"]="fs"
    ["lab9"]="mmap"
)

declare -A LAB_FOLDER_MAP=(
    ["lab1"]="lab1_util"
    ["lab2"]="lab2_syscall"
    ["lab3"]="lab3_pgtbl"
    ["lab4"]="lab4_traps"
    ["lab5"]="lab5_cow"
    ["lab6"]="lab6_net"
    ["lab7"]="lab7_lock"
    ["lab8"]="lab8_fs"
    ["lab9"]="lab9_mmap"
)

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

validate_lab() {
    local lab=$1
    if [[ ! ${LAB_BRANCH_MAP[$lab]+_} ]]; then
        print_error "Invalid lab number: $lab"
        print_info "Supported lab numbers: ${!LAB_BRANCH_MAP[@]}"
        exit 1
    fi
}

check_directories() {
    if [[ ! -d "$XV6_REPO" ]]; then
        print_error "xv6-labs-2024 directory does not exist: $XV6_REPO"
        exit 1
    fi
    
    if [[ ! -d "$MIT_LABS" ]]; then
        print_error "mit-6.828 directory does not exist: $MIT_LABS"
        exit 1
    fi
    
    mkdir -p "$PATCHES_DIR"
}

checkout_branch() {
    local lab=$1
    local branch=${LAB_BRANCH_MAP[$lab]}
    
    print_info "Switching to branch for lab $lab: $branch"
    
    cd "$XV6_REPO"
    
    if ! git show-ref --verify --quiet refs/heads/$branch; then
        print_error "Branch $branch does not exist"
        exit 1
    fi
    
    git checkout $branch
    print_success "Switched to branch $branch"
}

export_patch() {
    local lab=$1
    local folder=${LAB_FOLDER_MAP[$lab]}
    local patch_file="$PATCHES_DIR/${lab}_patch.diff"
    
    print_info "Exporting patch file for lab $lab"
    
    if [[ ! -d "$MIT_LABS/$folder" ]]; then
        print_error "Target folder does not exist: $MIT_LABS/$folder"
        exit 1
    fi
    
    print_info "Generating diff between your lab folder and original branch..."
    
    local temp_dir=$(mktemp -d)
    cd "$XV6_REPO"
    
    mkdir -p "$temp_dir/original_branch"
    
    git checkout ${LAB_BRANCH_MAP[$lab]}
    git archive --format=tar HEAD | tar -x -C "$temp_dir/original_branch"
    
    cp -r "$MIT_LABS/$folder" "$temp_dir/my_lab"
    
    find "$temp_dir/original_branch" -name "*.o" -o -name "*.d" -o -name "*.asm" -o -name "*.sym" -o -name "*.img" -o -name "_*" -o -name "*.out*" -o -wholename "*/kernel/kernel" -o -wholename "*/mkfs/mkfs" -o -name "__pycache__" -o -name ".cache" -o -name "usys.S" -o -name "compile_commands.json" -o -name ".gdbinit" -o -name "initcode" -o -name "time.txt" | while read file; do
        rm -rf "$file"
    done
    
    find "$temp_dir/my_lab" -name "*.o" -o -name "*.d" -o -name "*.asm" -o -name "*.sym" -o -name "*.img" -o -name "_*" -o -name "*.out*" -o -wholename "*/kernel/kernel" -o -wholename "*/mkfs/mkfs" -o -name "__pycache__" -o -name ".cache" -o -name "usys.S" -o -name "compile_commands.json" -o -name ".gdbinit" -o -name "initcode" -o -name "time.txt" | while read file; do
        rm -rf "$file"
    done
    
    cd "$temp_dir"
    
    diff -u -r -N original_branch my_lab > "$patch_file" || true
    
    rm -rf "$temp_dir"
    
    if [[ -s "$patch_file" ]]; then
        print_success "Patch file generated: $patch_file"
        print_info "Patch file size: $(wc -l < "$patch_file") lines"
    else
        print_warning "No differences detected, patch file is empty"
    fi
}

apply_patch() {
    local source_lab=$1
    local target_lab=$2
    local patch_file="$PATCHES_DIR/${source_lab}_patch.diff"
    local target_folder=${LAB_FOLDER_MAP[$target_lab]}
    
    print_info "Applying patch from $source_lab to $target_lab"
    
    if [[ ! -f "$patch_file" ]]; then
        print_error "Patch file does not exist: $patch_file"
        print_info "Please run first: $0 export $source_lab"
        exit 1
    fi
    
    if [[ ! -d "$MIT_LABS/$target_folder" ]]; then
        print_error "Target folder does not exist: $MIT_LABS/$target_folder"
        exit 1
    fi
    
    cd "$MIT_LABS/$target_folder"
    
    local backup_dir="$MIT_LABS/${target_folder}_backup_$(date +%Y%m%d_%H%M%S)"
    print_info "Backing up current state to: $backup_dir"
    cp -r . "$backup_dir"
    
    print_info "Applying patch file: $patch_file"
    if patch -p1 < "$patch_file"; then
        print_success "Patch applied successfully"
        print_info "Backup location: $backup_dir"
    else
        print_error "Patch application failed"
        print_info "Please check conflicts or resolve manually"
        print_info "Backup location: $backup_dir"
        exit 1
    fi
}

show_help() {
    echo "xv6 lab sync script"
    echo ""
    echo "Usage:"
    echo "  $0 checkout <lab_number>                    # Switch to corresponding branch"
    echo "  $0 export <lab_number>                      # Export lab's patch file"
    echo "  $0 apply <source_lab> <target_lab>         # Apply patch to target lab"
    echo "  $0 list                                     # List all supported labs"
    echo "  $0 help                                     # Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 checkout lab1                            # Switch to util branch"
    echo "  $0 export lab2                              # Export lab2's patch"
    echo "  $0 apply lab2 lab3                          # Apply lab2's patch to lab3"
    echo ""
    echo "Supported lab numbers:"
    for lab in "${!LAB_BRANCH_MAP[@]}"; do
        echo "  $lab -> ${LAB_BRANCH_MAP[$lab]} (${LAB_FOLDER_MAP[$lab]})"
    done
}

list_labs() {
    echo "Supported lab numbers:"
    echo ""
    printf "%-8s %-12s %-20s\n" "Lab" "Branch" "Folder"
    echo "----------------------------------------"
    for i in {1..9}; do
        lab="lab$i"
        if [[ ${LAB_BRANCH_MAP[$lab]+_} ]]; then
            printf "%-8s %-12s %-20s\n" "$lab" "${LAB_BRANCH_MAP[$lab]}" "${LAB_FOLDER_MAP[$lab]}"
        fi
    done
}

main() {
    check_directories
    
    case "${1:-help}" in
        "checkout")
            if [[ -z "$2" ]]; then
                print_error "Please specify lab number"
                show_help
                exit 1
            fi
            validate_lab "$2"
            checkout_branch "$2"
            ;;
        "export")
            if [[ -z "$2" ]]; then
                print_error "Please specify lab number"
                show_help
                exit 1
            fi
            validate_lab "$2"
            export_patch "$2"
            ;;
        "apply")
            if [[ -z "$2" || -z "$3" ]]; then
                print_error "Please specify source and target lab numbers"
                show_help
                exit 1
            fi
            validate_lab "$2"
            validate_lab "$3"
            apply_patch "$2" "$3"
            ;;
        "list")
            list_labs
            ;;
        "help"|"-h"|"--help")
            show_help
            ;;
        *)
            print_error "Unknown command: $1"
            show_help
            exit 1
            ;;
    esac
}

main "$@"
